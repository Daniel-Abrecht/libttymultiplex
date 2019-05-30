// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <libttymultiplex.h>

/** \file */

int tym_pane_set_flag(int pane, enum tym_flag flag, bool state){
  int ret = 0;
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
  switch(flag){
    case TYM_PF_FOCUS: ret = tym_i_pane_focus(ppane); break;
    case TYM_PF_DISALLOW_FOCUS: {
      if(!state && tym_i_focus_pane == ppane)
        tym_i_pane_focus(0);
      ppane->nofocus = state;
    } break;
  }
  pthread_mutex_unlock(&tym_i_lock);
  return ret;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_get_flag(int pane, enum tym_flag flag){
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
  switch(flag){
    case TYM_PF_FOCUS: return tym_i_focus_pane == ppane;
    case TYM_PF_DISALLOW_FOCUS: return ppane->nofocus;
  }
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}
