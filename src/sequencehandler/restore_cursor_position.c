// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_restore_cursor_position(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 0){
    errno = ENOENT;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_ABSOLUTE, screen->saved_cursor.x,
    TYM_I_SCP_SMM_NO_SCROLLING, TYM_I_SCP_PM_ABSOLUTE, screen->saved_cursor.y,
    TYM_I_SCP_SCROLLING_REGION_IRRELEVANT, false
  );
  return 0;
}
