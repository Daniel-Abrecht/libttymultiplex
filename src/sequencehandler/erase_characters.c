// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <curses.h>
#include <internal/pane.h>

int tym_i_csq_erase_characters(struct tym_i_pane_internal* pane){
  size_t n = 1;
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  if(pane->sequence.integer_count > 0)
    n = pane->sequence.integer[0];
  unsigned x = pane->cursor.x;
  unsigned y = pane->cursor.y;
  for(size_t i=0; i<n; i++){
    mvwaddch(pane->window, y, x, ' ');
    if(++x >= w){
      x = 0;
      if(++y >= h)
        break;
    }
  }
  return 0;
}
