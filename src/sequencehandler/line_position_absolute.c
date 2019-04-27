// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_line_position_absolute(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  if(pane->sequence.integer_count <= 0)
    pane->sequence.integer[0] = 1;
  unsigned y = pane->sequence.integer[0] - 1;
  tym_i_pane_cursor_set_cursor(pane, screen->cursor.x, y, TYM_I_SMB_IGNORE);
  return 0;
}
