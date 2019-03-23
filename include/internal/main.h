// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_MAIN_H
#define TYM_INTERNAL_MAIN_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <pty.h>

enum tym_i_special_tym_i_poll_fds {
  SPF_POLLCTLFD,
  SPF_SIGNALFD,
  SPF_TERMINPUT,
  SPF_COUNT
};

enum tym_i_init_state {
  INIT_STATE_NOINIT,
  INIT_STATE_INITIALISED,
  INIT_STATE_SHUTDOWN_IN_PROGRESS,
  INIT_STATE_SHUTDOWN = INIT_STATE_NOINIT
};

enum tym_i_poll_ctl_type {
  TYM_PC_ADD,
  TYM_PC_REMOVE,
};

struct tym_i_poll_ctl {
  enum tym_i_poll_ctl_type action;
  union {
    struct {
      int fd;
    } add;
    struct {
      int fd;
    } remove;
  } data;
};

extern enum tym_i_init_state tym_i_binit;
extern size_t tym_i_poll_fdn;
extern struct pollfd* tym_i_poll_fds;
extern int tym_i_tty;
extern int tym_i_sfd;
extern int tym_i_pollctl[2];
extern pthread_t tym_i_main_loop;
extern struct winsize tym_i_ttysize;
extern pthread_mutexattr_t tym_i_lock_attr;
extern pthread_mutex_t tym_i_lock; /* reentrant mutex */

void* tym_i_main(void* ptr);
int tym_i_pollfd_add_sub(struct pollfd pfd);
int tym_i_pollfd_add(int fd);
int tym_i_pollfd_remove(int fd);
void tym_i_debug(const char* format, ...);

#endif
