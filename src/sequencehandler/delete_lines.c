// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_delete_lines(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  int n = 1;
  if(pane->sequence.integer_count)
    n = pane->sequence.integer[0];
  tym_i_pane_insert_delete_lines(pane, screen->cursor.y, n);
  return 0;
}
