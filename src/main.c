// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ncurses.h>
#include <pty.h>
#include <sys/signalfd.h>
#include <internal/list.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/backend.h>
#include <internal/pseudoterminal.h>
#include <libttymultiplex.h>

enum tym_i_init_state tym_i_binit = INIT_STATE_NOINIT;
size_t tym_i_poll_fdn;
struct pollfd* tym_i_poll_fds;
int tym_i_tty;
int tym_i_sfd;
int tym_i_pollctl[2];
struct tym_absolute_position_rectangle tym_i_bounds;
pthread_t tym_i_main_loop;
pthread_mutexattr_t tym_i_lock_attr;
pthread_mutex_t tym_i_lock; /* reentrant mutex */

const struct tym_special_key_name tym_special_key_list[] = {
#define X(ID, VAL) \
  { \
    .key  = TYM_KEY_ ## ID, \
    .name = #ID, \
    .name_length = sizeof(#ID)-1, \
  },
TYM_I_SPECIAL_KEYS
#undef X
};
const size_t tym_special_key_count = sizeof(tym_special_key_list) / sizeof(*tym_special_key_list);

static FILE* tym_i_debugfd;

static void init(void) __attribute__((constructor,used));
static void init(void){
  {
    const char* debugfd = getenv("TM_DEBUGFD");
    if(debugfd){
      int fd = atoi(debugfd);
      if(fd >= 0){
        fd = dup(fd);
        if(fd != -1)
          tym_i_debugfd = fdopen(fd,"a");
      }
    }
  }
  pthread_mutexattr_init(&tym_i_lock_attr);
  pthread_mutexattr_settype(&tym_i_lock_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutexattr_setpshared(&tym_i_lock_attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&tym_i_lock, &tym_i_lock_attr);
}

static void shutdown(void) __attribute__((destructor,used));
static void shutdown(void){
  tym_shutdown();
}

int tym_i_pollfd_add_sub(struct pollfd pfd){
  return tym_i_list_add(sizeof(*tym_i_poll_fds), &tym_i_poll_fdn, (void**)&tym_i_poll_fds, &pfd);
}

int tym_i_pollfd_remove_sub(size_t entry){
  if(entry >= tym_i_poll_fdn){
    errno = EINVAL;
    return -1;
  }
  close(tym_i_poll_fds[entry].fd);
  return tym_i_list_remove(sizeof(*tym_i_poll_fds), &tym_i_poll_fdn, (void**)&tym_i_poll_fds, entry);
}

int tym_i_pollfd_add(int fd){
  struct tym_i_poll_ctl ctl = {
    .action = TYM_PC_ADD,
    .data.add.fd = fd
  };
  ssize_t ret = 0;
  while((ret=write(tym_i_pollctl[1], &ctl, sizeof(ctl))) == -1 && errno == EINTR);
  if(ret == -1)
    return -1;
  return 0;
}

int tym_i_pollfd_remove(int fd){
  struct tym_i_poll_ctl ctl = {
    .action = TYM_PC_REMOVE,
    .data.remove.fd = fd
  };
  ssize_t ret = 0;
  while((ret=write(tym_i_pollctl[1], &ctl, sizeof(ctl))) == -1 && errno == EINTR);
  if(ret == -1)
    return -1;
  return 0;
}

void* tym_i_main(void* ptr){
  (void)ptr;
  while(true){
    int ret = poll(tym_i_poll_fds, tym_i_poll_fdn, -1);
    pthread_mutex_lock(&tym_i_lock);
    if(ret == -1){
      if(errno == EINTR)
        goto cont;
      perror("poll failed");
      goto error;
    }
    if(tym_i_poll_fds[SPF_POLLCTLFD].revents & (POLLHUP|POLLERR|POLLNVAL)){
      pthread_mutex_unlock(&tym_i_lock);
      break;
    }
    if(tym_i_poll_fds[SPF_POLLCTLFD].revents & POLLIN){
      struct tym_i_poll_ctl ctl = {0};
      ssize_t s;
      while((s=read(tym_i_pollctl[0], &ctl, sizeof(ctl))) == -1 && errno == EINTR);
      if(s == -1){
        perror("read failed");
        goto error;
      }
      switch(ctl.action){
        case TYM_PC_ADD: {
          tym_i_pollfd_add_sub((struct pollfd){
            .fd = ctl.data.add.fd,
            .events = POLLIN | POLLHUP
          });
        } break;
        case TYM_PC_REMOVE: {
          for(size_t i=SPF_COUNT; i<tym_i_poll_fdn; i++)
            if(tym_i_poll_fds[i].fd == ctl.data.remove.fd){
              tym_i_pollfd_remove_sub(i);
              break;
            }
        } break;
      }
    }else if(tym_i_poll_fds[SPF_POLLCTLFD].revents){
      pthread_mutex_unlock(&tym_i_lock);
      break;
    }
    if(tym_i_poll_fds[SPF_SIGNALFD].revents & POLLIN){
      struct signalfd_siginfo info;
      ssize_t s = read(tym_i_sfd, &info, sizeof(info));
      if(s != sizeof(info)){
        perror("read failed");
        goto error;
      }
      switch(info.ssi_signo){
        case SIGWINCH: tym_i_pane_update_size_all(); break;
        case SIGSTOP:
        case SIGTSTP:
        case SIGINT: {
          if(!tym_i_focus_pane)
            break;
#if defined(TIOCSIG)
            ioctl(tym_i_focus_pane->master, TIOCSIG, info.ssi_signo);
#elif defined(TIOCSIGNAL)
          if(tym_i_focus_pane)
            ioctl(tym_i_focus_pane->master, TIOCSIGNAL, info.ssi_signo);
#endif
        } break;
      }
    }else if(tym_i_poll_fds[SPF_SIGNALFD].revents){
      pthread_mutex_unlock(&tym_i_lock);
      break;
    }
    if(tym_i_poll_fds[SPF_TERMINPUT].revents & (POLLHUP|POLLERR|POLLNVAL)){
      pthread_mutex_unlock(&tym_i_lock);
      break;
    }else if(tym_i_poll_fds[SPF_TERMINPUT].revents & POLLIN){
      // TODO: abstract this away into the backend too
      int c = getch();
      if(c != ERR)
      switch(c){
        case KEY_MOUSE: {
          MEVENT event;
          if(getmouse(&event) != OK)
            break;
          for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next){
            int left   = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_LEFT  );
            int right  = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_RIGHT );
            int top    = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_TOP   );
            int bottom = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_BOTTOM);
            if( left > (int)event.x || right <= (int)event.x || top > (int)event.y || bottom <= (int)event.y )
              continue;
            unsigned x = event.x - left;
            unsigned y = event.y - top;
            tym_i_pane_focus(it);
            if(event.bstate & (BUTTON1_RELEASED | BUTTON2_RELEASED | BUTTON3_RELEASED)){
              tym_i_pts_send_mouse_event(it, TYM_BUTTON_RELEASED, (struct tym_i_cell_position){.x=x, .y=y});
            }
            if(event.bstate & BUTTON1_PRESSED){
              tym_i_pts_send_mouse_event(it, TYM_BUTTON_LEFT_PRESSED, (struct tym_i_cell_position){.x=x, .y=y});
            }
            if(event.bstate & BUTTON2_PRESSED){
              tym_i_pts_send_mouse_event(it, TYM_BUTTON_MIDDLE_PRESSED, (struct tym_i_cell_position){.x=x, .y=y});
            }
            if(event.bstate & BUTTON3_PRESSED){
              tym_i_pts_send_mouse_event(it, TYM_BUTTON_RIGHT_PRESSED, (struct tym_i_cell_position){.x=x, .y=y});
            }
            break;
          }
        } break;
        case KEY_ENTER: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_ENTER); break;
        case KEY_UP   : tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_UP); break;
        case KEY_DOWN : tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_DOWN); break;
        case KEY_RIGHT: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_RIGHT); break;
        case KEY_LEFT : tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_LEFT); break;
        case KEY_BACKSPACE: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_BACKSPACE); break;
        case KEY_HOME: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_HOME); break;
        case KEY_END: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_END); break;
        case KEY_DC: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_DELETE); break;
        default: tym_i_pts_send_key(tym_i_focus_pane, c); break;
      }
    }else if(tym_i_poll_fds[SPF_TERMINPUT].revents){
      pthread_mutex_unlock(&tym_i_lock);
      break;
    }
    for(size_t i=SPF_COUNT; i<tym_i_poll_fdn; i++){
      if(!tym_i_poll_fds[i].revents)
        continue;
      if(tym_i_poll_fds[i].revents & (POLLERR|POLLNVAL)){
        tym_i_pollfd_remove_sub(i);
      }else if(tym_i_poll_fds[i].revents & POLLIN){
        static char buf[256];
        ssize_t ret;
        do {
          ret = read(tym_i_poll_fds[i].fd, buf, sizeof(buf));
        } while(ret == -1 && errno == EINTR);
        if(ret == -1 && errno == EBADF)
          tym_i_pollfd_remove_sub(i);
        if(ret > 0)
        for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next){
          if(it->master != tym_i_poll_fds[i].fd)
            continue;
          for(size_t i=0; i<(size_t)ret; i++)
            tym_i_pane_parse(it, buf[i]);
          tym_i_backend->pane_refresh(it);
          break;
        }
      }else{
        tym_i_pollfd_remove_sub(i);
      }
    }
  cont:
    pthread_mutex_unlock(&tym_i_lock);
  }
  pthread_mutex_lock(&tym_i_lock);
  while(tym_i_poll_fdn)
    tym_i_pollfd_remove_sub(0);
  tym_i_backend->cleanup();
  tym_i_binit = INIT_STATE_SHUTDOWN;
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  abort();
}

void tym_i_perror(const char* x){
  tym_i_debug("%s: %d %s\n", x, errno, strerror(errno));
}

void tym_i_debug(const char* format, ...){
  va_list args;
  va_start(args, format);
  if(tym_i_debugfd)
    vfprintf(tym_i_debugfd, format, args);
  va_end(args);
}
