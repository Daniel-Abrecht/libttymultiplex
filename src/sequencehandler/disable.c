// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>


int tym_i_csq_disable(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 1){
    errno = EINVAL;
    return -1;
  }
  switch(pane->sequence.integer[0]){
    case TYM_I_DSDR_MOUSE_MODE_X10:
    case TYM_I_DSDR_MOUSE_MODE_NORMAL:
    case TYM_I_DSDR_MOUSE_MODE_BUTTON:
    case TYM_I_DSDR_MOUSE_MODE_ANY: pane->mouse_mode = MOUSE_MODE_OFF; break;
  }
  return 0;
}
