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
#include <locale.h>
#include <pthread.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/calc.h>
#include <internal/backend.h>
#include <libttymultiplex.h>

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
  memset(&tym_i_bounds, 0, sizeof(tym_i_bounds));
  tym_i_calc_init_absolute_position(&tym_i_bounds.edge[TYM_RECT_TOP_LEFT]);
  tym_i_calc_init_absolute_position(&tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT]);
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], RATIO, TYM_AXIS_HORIZONTAL) = 1;
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], RATIO, TYM_AXIS_VERTICAL) = 1;
  tym_i_binit = INIT_STATE_INITIALISED;
  tym_i_tty = dup(STDIN_FILENO);
  setlocale(LC_CTYPE, "C.UTF-8");
  if(tym_i_backend_init(getenv("TM_BACKEND")) != 0){
    fprintf(stderr,"Failed to initialise backend\n");
    goto error;
  }
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGWINCH);
  sigaddset(&sigmask, SIGINT);
  if(pthread_sigmask(SIG_BLOCK, &sigmask, 0) == -1)
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
  (*handler)(ptr, pane, &ppane->super_position, &ppane->absolute_position);
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
