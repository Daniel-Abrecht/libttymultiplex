// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pty.h>
#include <sys/signalfd.h>
#include <internal/list.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/pseudoterminal.h>
#include <libttymultiplex.h>

enum tym_i_init_state tym_i_binit = INIT_STATE_NOINIT;
size_t tym_i_poll_fdn;
struct pollfd* tym_i_poll_fds;
int tym_i_tty;
int tym_i_sfd;
int tym_i_pollctl[2];
struct winsize tym_i_ttysize;
mmask_t tym_i_mouseeventmask;
pthread_t tym_i_main_loop;
pthread_mutexattr_t tym_i_lock_attr;
pthread_mutex_t tym_i_lock; /* reentrant mutex */

int colortable8[9] = {
  -1, // default color
  COLOR_BLACK,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW,
  COLOR_BLUE,
  COLOR_MAGENTA,
  COLOR_CYAN,
  COLOR_WHITE
};

static FILE* tym_i_debugfd;

static void init(void) __attribute__((constructor,used));
static void init(void){
  {
    const char* debugfd = getenv("DEBUGFD");
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
  pthread_mutex_init(&tym_i_lock, &tym_i_lock_attr);
}

enum tym_unit_type tym_positon_unit_map[] = {
  [TYM_P_CHARFIELD] = TYM_U_INTEGER,
  [TYM_P_RATIO] = TYM_U_REAL,
};

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
    if(tym_i_poll_fds[SPF_POLLCTLFD].revents & POLLHUP){
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
            .events = POLLIN | POLLHUP | POLLPRI
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
    }
    if(tym_i_poll_fds[SPF_TERMINPUT].revents & POLLHUP){
      // TODO
    }
    if(tym_i_poll_fds[SPF_TERMINPUT].revents & POLLPRI){
      // TODO
    }
    if(tym_i_poll_fds[SPF_TERMINPUT].revents & POLLIN){
      while(true){
        int c = getch();
        if(c == ERR)
          break;
        switch(c){
          case KEY_MOUSE: {
            MEVENT event;
            if(getmouse(&event) != OK)
              break;
            for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next){
              if( it->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer >  event.x
               || it->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer <= event.x
               || it->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer >  event.y
               || it->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer <= event.y
              ) continue;
              unsigned x = event.x - it->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
              unsigned y = event.y - it->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
              tym_i_pane_focus(it);
              tym_i_pts_send_mouse_event(it, event.bstate, x, y);
              break;
            }
          } break;
          case KEY_ENTER: tym_i_pts_send(tym_i_focus_pane, 1, "\n"); break;
          case KEY_UP   : tym_i_pts_send(tym_i_focus_pane, 3, "\x1B[A"); break;
          case KEY_DOWN : tym_i_pts_send(tym_i_focus_pane, 3, "\x1B[B"); break;
          case KEY_RIGHT: tym_i_pts_send(tym_i_focus_pane, 3, "\x1B[C"); break;
          case KEY_LEFT : tym_i_pts_send(tym_i_focus_pane, 3, "\x1B[D"); break;
          case KEY_BACKSPACE: tym_i_pts_send_key('\b'); break;
          default: tym_i_pts_send_key(c); break;
        }
      };
    }
    for(size_t i=SPF_COUNT; i<tym_i_poll_fdn; i++){
      if(!tym_i_poll_fds[i].revents)
        continue;
      if(tym_i_poll_fds[i].revents & POLLIN){
        while(true){
          static char buf[256];
          ssize_t ret = read(tym_i_poll_fds[i].fd, buf, sizeof(buf));
          if(ret == -1 && errno == EINTR)
            continue;
          if((ret == -1 && errno == EAGAIN) || !ret)
            break;
          for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next){
            if(it->master != tym_i_poll_fds[i].fd)
              continue;
            for(size_t i=0; i<(size_t)ret; i++)
              tym_i_pane_parse(it, buf[i]);
            wrefresh(it->window);
            break;
          }
          if((size_t)ret < sizeof(buf))
            break;
        }
      }
    }
  cont:
    pthread_mutex_unlock(&tym_i_lock);
  }
  pthread_mutex_lock(&tym_i_lock);
  while(tym_i_poll_fdn)
    tym_i_pollfd_remove_sub(0);
  tym_i_binit = INIT_STATE_SHUTDOWN;
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  abort();
}

void tym_i_debug(const char* format, ...){
  va_list args;
  va_start(args, format);
  if(tym_i_debugfd)
    vfprintf(tym_i_debugfd, format, args);
  va_end(args);
}
