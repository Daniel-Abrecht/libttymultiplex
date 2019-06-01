// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <ncurses.h>
#include <pty.h>
#include <sys/signalfd.h>
#include <internal/list.h>
#include <internal/main.h>
#include <internal/backend.h>
#include <internal/pseudoterminal.h>
#include <libttymultiplex.h>

/** \file */

enum tym_i_init_state tym_i_binit = INIT_STATE_NOINIT;
size_t tym_i_poll_count;
struct pollfd* tym_i_poll_list;
struct tym_i_pollfd_complement* tym_i_poll_list_complement;
int tym_i_tty;
int tym_i_sfd;
int tym_i_pollctl[2];
struct tym_absolute_position_rectangle tym_i_bounds;
pthread_t tym_i_main_loop;
pthread_mutexattr_t tym_i_lock_attr;
pthread_mutex_t tym_i_lock;

size_t tym_i_resize_handler_count;
struct tym_i_resize_handler_ptr_pair* tym_i_resize_handler_list;


#define Y1(ID, VAL) \
  { \
    .key  = TYM_KEY_ ## ID, \
    .name = #ID, \
    .name_length = sizeof(#ID)-1, \
  },
const struct tym_special_key_name tym_special_key_list[] = { TYM_I_SPECIAL_KEYS(Y1) };
#undef Y1
const size_t tym_special_key_count = sizeof(tym_special_key_list) / sizeof(*tym_special_key_list);

static FILE* tym_i_debugfd;

/** Open the debug file descriptor and initialise mutex attributes. This is done even before main. */
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
  pthread_mutex_init(&tym_i_lock, &tym_i_lock_attr);
}

/** This is automatically called before the program exits. This is to make sure everything always gets properly deinitialised. */
static void shutdown(void) __attribute__((destructor,used));
static void shutdown(void){
  tym_shutdown();
}

/** Add a rseize handler */
int tym_i_resize_handler_add(const struct tym_i_resize_handler_ptr_pair* cp){
  return tym_i_list_add(sizeof(*tym_i_resize_handler_list), &tym_i_resize_handler_count, (void**)&tym_i_resize_handler_list, cp);
}

/** Remove a resize handler */
int tym_i_resize_handler_remove(size_t entry){
  return tym_i_list_remove(sizeof(*tym_i_resize_handler_list), &tym_i_resize_handler_count, (void**)&tym_i_resize_handler_list, entry);
}

/**
 * Add the poll file descriptor. This should only be used within the main thread or before it is started.
 * In all other cases, use tym_i_pollfd_add, which will delegate this task to the main loop.
 */
int tym_i_pollfd_add_sub(struct pollfd pfd, const struct tym_i_pollfd_complement* complement){
  size_t n1 = tym_i_poll_count;
  if(tym_i_list_add(sizeof(*tym_i_poll_list), &n1, (void**)&tym_i_poll_list, &pfd) == -1){
    assert(n1 == tym_i_poll_count); // Adding an entry failed, the number of list entries should have stayed the same.
    return -1;
  }
  size_t n2 = tym_i_poll_count;
  if(tym_i_list_add(sizeof(*tym_i_poll_list_complement), &n2, (void**)&tym_i_poll_list_complement, complement) == -1){
    tym_i_list_remove(sizeof(*tym_i_poll_list), &n1, (void**)&tym_i_poll_list, tym_i_poll_count);
    assert(n1 == n2); // These lists must always have the same length
    assert(n1 == tym_i_poll_count); // The number of list entries should have been the same as at the beginning.
    return -1;
  }
  assert(n1 == n2); // These lists must always have the same length
  assert(n1 == tym_i_poll_count+1); // Exactly one entry should have been added to both lists.
  tym_i_poll_count = n1;
  return 0;
}

/**
 * Removes a poll file descriptor using its list index. Only to be used in the main thread.
 * In all other cases, use tym_i_pollfd_remove, which will delegate this task to the main loop.
 */
static int tym_i_pollfd_remove_sub(size_t entry){
  if(entry >= tym_i_poll_count){
    errno = EINVAL;
    return -1;
  }
  struct tym_i_pollfd_complement* plc = &tym_i_poll_list_complement[entry];
  if(plc->onremove)
    tym_i_poll_list_complement[entry].onremove(tym_i_poll_list_complement[entry].ptr, tym_i_poll_list[entry].fd);
  close(tym_i_poll_list[entry].fd);
  size_t n1 = tym_i_poll_count;
  size_t n2 = tym_i_poll_count;
  bool ok = true;
  bool res;
  res = tym_i_list_remove(sizeof(*tym_i_poll_list), &n1, (void**)&tym_i_poll_list, entry);
  ok = ok && res;
  res = tym_i_list_remove(sizeof(*tym_i_poll_list_complement), &n2, (void**)&tym_i_poll_list_complement, entry);
  ok = ok && res;
  assert(n1 == n2); // These lists must always have the same length
  assert(n1 <= tym_i_poll_count); // These lists can't have increased in size
  ok = ok && tym_i_poll_count-1 == n1;
  tym_i_poll_count = n1;
  return -!ok;
}

/** Add a file descriptor to be polled. */
int tym_i_pollfd_add(int fd, const struct tym_i_pollfd_complement* complement){
  if(fd < 1 || !complement->onevent){
    errno = EINVAL;
    return -1;
  }
  struct tym_i_poll_ctl ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.action = TYM_PC_ADD,
  ctl.data.add.fd = fd;
  ctl.data.add.complement = *complement;
  ssize_t ret = 0;
  while((ret=write(tym_i_pollctl[1], &ctl, sizeof(ctl))) == -1 && errno == EINTR);
  if(ret == -1)
    return -1;
  return 0;
}

/** Remove the file descriptor from the ones watched by the main loop. */
int tym_i_pollfd_remove(int fd){
  struct tym_i_poll_ctl ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.action = TYM_PC_REMOVE;
  ctl.data.remove.fd = fd;
  for(size_t i=0; i<tym_i_poll_count; i++)
    if(tym_i_poll_list[i].fd == fd)
      tym_i_poll_list_complement[i].onevent = 0;
  ssize_t ret = 0;
  while((ret=write(tym_i_pollctl[1], &ctl, sizeof(ctl))) == -1 && errno == EINTR);
  if(ret == -1)
    return -1;
  return 0;
}

/** Send the main loop the command to exit. */
int tym_i_request_freeze(void){
  enum tym_i_init_state init = tym_i_binit;
  tym_i_binit = INIT_STATE_FREEZE_IN_PROGRESS;
  struct tym_i_poll_ctl ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.action = TYM_PC_FREEZE;
  ssize_t ret = 0;
  while((ret=write(tym_i_pollctl[1], &ctl, sizeof(ctl))) == -1 && errno == EINTR);
  if(ret == -1){
    tym_i_binit = init;
    return -1;
  }
  return 0;
}

/** Update the size & position of all panes and everything. Usually done after the terminal size changes. */
int tym_i_update_size_all(void){
  if(tym_i_backend->update_terminal_size_information() == -1)
    return -1;
  for(size_t i=0; i<tym_i_resize_handler_count; i++){
    struct tym_i_resize_handler_ptr_pair* cp = tym_i_resize_handler_list + i;
    cp->callback(cp->ptr, &tym_i_bounds);
  }
  tym_i_backend->resize();
  for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next)
    tym_i_pane_update_size(it);
  return 0;
}

int tym_i_pollhandler_ctl_command_handler(void* ptr, short event, int fd){
  if(!(event & POLLIN))
    return -1;
  (void)ptr;
  struct tym_i_poll_ctl ctl = {0};
  ssize_t s;
  while((s=read(fd, &ctl, sizeof(ctl))) == -1 && errno == EINTR);
  if(s == -1){
    perror("read failed");
    return -1;
  }
  switch(ctl.action){
    case TYM_PC_ADD: {
      tym_i_pollfd_add_sub((struct pollfd){
        .fd = ctl.data.add.fd,
        .events = POLLIN | POLLHUP
      }, &ctl.data.add.complement);
    } break;
    case TYM_PC_REMOVE: {
      for(size_t i=0; i<tym_i_poll_count; i++){
        if(tym_i_poll_list[i].fd != ctl.data.remove.fd)
          continue;
        tym_i_pollfd_remove_sub(i);
        break;
      }
    } break;
    case TYM_PC_FREEZE: {
      if(tym_i_binit == INIT_STATE_FREEZE_IN_PROGRESS)
        tym_i_binit = INIT_STATE_FROZEN;
    } break;
  }
  return 0;
}

int tym_i_pollhandler_signal_handler(void* ptr, short event, int fd){
  if(!(event & POLLIN))
    return -1;
  (void)ptr;
  struct signalfd_siginfo info;
  ssize_t s = read(fd, &info, sizeof(info));
  if(s != sizeof(info)){
    perror("read failed");
    return -1;
  }
  switch(info.ssi_signo){
    case SIGWINCH: tym_i_update_size_all(); break;
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
  return 0;
}

int tym_i_pollhandler_terminal_input_handler(void* ptr, short event, int fd){
  (void)fd;
  if(!(event & POLLIN))
    return -1;
  (void)ptr;
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
  return 0;
}

/** The main loop */
void* tym_i_main(void* ptr){
  (void)ptr;
  while(tym_i_poll_count){
    int ret = poll(tym_i_poll_list, tym_i_poll_count, -1);
    pthread_mutex_lock(&tym_i_lock);
    if(ret == -1){
      if(errno == EINTR)
        goto cont;
      perror("poll failed");
      goto shutdown;
    }
    if( tym_i_binit != INIT_STATE_INITIALISED
     && tym_i_binit != INIT_STATE_FREEZE_IN_PROGRESS
    ) goto shutdown;
    for(size_t i=0; i<tym_i_poll_count; i++){
      struct pollfd* pfd = &tym_i_poll_list[i];
      struct tym_i_pollfd_complement* pc = &tym_i_poll_list_complement[i];
      if(!pfd->revents)
        continue;
      if( pfd->revents & (POLLERR|POLLNVAL) || !pc->onevent
       || pc->onevent(pc->ptr, pfd->revents, pfd->fd) == -1
      ){
        if(pc->onremove)
          pc->onremove(pc->ptr, pfd->fd);
        tym_i_pollfd_remove_sub(i);
        continue;
      }
      if(tym_i_binit == INIT_STATE_FROZEN){
        goto exit;
      }else if( tym_i_binit != INIT_STATE_INITIALISED
             && tym_i_binit != INIT_STATE_FREEZE_IN_PROGRESS
      ) goto shutdown;
    }
  cont:
    pthread_mutex_unlock(&tym_i_lock);
  }
shutdown:
  while(tym_i_poll_count)
    tym_i_pollfd_remove_sub(0);
  tym_i_backend->cleanup();
  close(tym_i_pollctl[1]);
  tym_i_binit = INIT_STATE_SHUTDOWN;
exit:
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
}

/**
 * Similar to perror, but prints output to tym_i_debugfd.
 * 
 * \see tym_i_debug
 */
void tym_i_error(const char* x){
  tym_i_debug("%s: %d %s\n", x, errno, strerror(errno));
}

/**
 * Output debug messages to the debug file descriptor.
 * The debug file descriptor can be set using the TM_DEBUGFD environment variable.
 */
void tym_i_debug(const char* format, ...){
  va_list args;
  va_start(args, format);
  if(tym_i_debugfd)
    vfprintf(tym_i_debugfd, format, args);
  va_end(args);
}
