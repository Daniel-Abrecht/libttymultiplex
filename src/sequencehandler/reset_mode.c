// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/main.h>
#include <internal/pane.h>


int tym_i_csq_reset_mode(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 1){
    errno = EINVAL;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  enum tym_i_setmode code = pane->sequence.integer[0];
  switch(code){
    case TYM_I_SM_INSERT: screen->insert_mode = false; break; // IRM
    case TYM_I_SM_KEYBOARD_ACTION: break;
    case TYM_I_SM_SEND_RECEIVE: break;
    case TYM_I_SM_AUTOMATIC_NEWLINE: break;
    default: {
      TYM_U_LOG(TYM_LOG_INFO, "Disable for unknown mode %d\n", code);
      errno = ENOSYS;
    } return -1;
  }
  return 0;
}
