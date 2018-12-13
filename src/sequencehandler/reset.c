// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <curses.h>
#include <internal/pane.h>

int tym_i_csq_reset(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 0){
    errno = ENOENT;
    return -1;
  }
  pane->fgcolor.index = 0;
  pane->bgcolor.index = 0;
  wclear(pane->window);
  tym_i_pane_cursor_set_cursor(pane, 0, 0);
  return 0;
}
