// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_cursor_right(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  unsigned x = 1;
  if(pane->sequence.integer_count)
    x = pane->sequence.integer[0];
  unsigned new_x = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer - 1;
  if(pane->cursor.x + x < new_x)
    new_x = pane->cursor.x + x;
  tym_i_pane_cursor_set_cursor(pane, new_x, pane->cursor.y);
  return 0;
}
