// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pty.h>
#include <ncursesw/curses.h>
#include <pthread.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <libttymultiplex.h>

static mmask_t tym_i_mouseeventmask;

uint8_t colorpair8x8_triangular_number_mirror_mapping_index[9][9];

static void shutdown(void) __attribute__((destructor,used));
static void shutdown(void){
  tym_shutdown();
}

int tym_init(void){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit == INIT_STATE_SHUTDOWN_IN_PROGRESS){
    errno = EAGAIN;
    goto error;
  }
  if(tym_i_binit != INIT_STATE_SHUTDOWN){
    errno = EINVAL;
    goto error;
  }
  tym_i_binit = INIT_STATE_INITIALISED;
  tym_i_tty = dup(STDIN_FILENO);
  if(!initscr())
    goto error;
  cbreak();
  nodelay(stdscr, true);
  noecho();
  keypad(stdscr, true);
  if(has_colors()){
    start_color();
    use_default_colors();
    if(COLOR_PAIRS >= 45){
      for(uint8_t i=0, j=0, k=1; k<=45; k++){
        colorpair8x8_triangular_number_mirror_mapping_index[i][j] = k | COLORPAIR_MAPPING_NEGATIVE_FLAG;
        colorpair8x8_triangular_number_mirror_mapping_index[j][i] = k;
        init_pair(k,colortable8[j],colortable8[i]);
        if(i == j){
          j += 1;
          i  = 0;
        }else{
          i += 1;
        }
      }
    }else if(COLOR_PAIRS >= 16){
      // TODO
    }
  }
  leaveok(stdscr, false);
  refresh();
  mousemask(ALL_MOUSE_EVENTS|REPORT_MOUSE_POSITION, &tym_i_mouseeventmask);
  mouseinterval(0);
  write(tym_i_tty,"\033[?1003h",8); // Mouse movement not reportet otherwise. curses/terminfo bug?
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGWINCH);
  sigaddset(&sigmask, SIGINT);
  if(sigprocmask(SIG_BLOCK, &sigmask, 0) == -1)
    goto error;
  if(pipe(tym_i_pollctl))
    goto error;
  fcntl(tym_i_pollctl[0], F_SETFD, FD_CLOEXEC);
  fcntl(tym_i_pollctl[1], F_SETFD, FD_CLOEXEC);
  if((tym_i_sfd = signalfd(-1, &sigmask, SFD_CLOEXEC)) == -1)
    goto error;
  if(tym_i_pollfd_add_sub((struct pollfd){
    .events = POLLIN,
    .fd = tym_i_pollctl[0]
  })) goto error;
  if(tym_i_pollfd_add_sub((struct pollfd){
    .events = POLLIN | POLLHUP,
    .fd = tym_i_sfd
  })) goto error;
  if(tym_i_pollfd_add_sub((struct pollfd){
    .events = POLLIN | POLLHUP | POLLPRI,
    .fd = tym_i_tty
  })) goto error;
  tym_i_pane_update_size_all();
  tym_i_binit = INIT_STATE_INITIALISED;
  pthread_create(&tym_i_main_loop, 0, tym_i_main, 0);
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
error:
  write(tym_i_tty,"\033[?1003l",8);
  mousemask(tym_i_mouseeventmask, 0);
  endwin();
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_shutdown(void){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  while(tym_i_pane_list_start)
    if(tym_pane_destroy(tym_i_pane_list_start->id)){
      perror("tym_pane_destroy failed");
      break;
    }
  close(tym_i_pollctl[1]);
  write(tym_i_tty,"\033[?1003l",8);
  mousemask(tym_i_mouseeventmask, 0);
  endwin();
  tym_i_binit = INIT_STATE_SHUTDOWN_IN_PROGRESS;
  pthread_mutex_unlock(&tym_i_lock);
  pthread_join(tym_i_main_loop, 0);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_register_resize_handler( int pane, void* ptr, tym_resize_handler_t handler ){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  if(!tym_i_pane_resize_handler_add(ppane, &(struct tym_i_handler_ptr_pair){
    .callback = handler,
    .ptr = ptr
  })) goto error;
  (*handler)(ptr, pane, &ppane->superposition, &ppane->coordinates);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_unregister_resize_handler( int pane, void* ptr, tym_resize_handler_t handler ){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next){
    for(size_t i=0; i<it->resize_handler_count; i++){
      struct tym_i_handler_ptr_pair* cp = it->resize_handler_list + i;
      if(!((pane <= 0 || pane == it->id) && (!ptr || ptr == cp->ptr) && (!handler || handler == cp->callback)))
        continue;
      if(tym_i_pane_resize_handler_remove(it, i))
        goto error;
      i -= 1;
    }
  }
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}
