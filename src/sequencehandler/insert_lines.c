// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_insert_lines(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  int n = 1;
  if(pane->sequence.integer_count)
    n = pane->sequence.integer[0];
  if(n <= 0) n = 1;
  tym_i_pane_insert_delete_lines(pane, screen->cursor.y, -n);
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_ABSOLUTE, 0,
    TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY, TYM_I_SCP_PM_RELATIVE, 0,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE, false
  );
  return 0;
}
