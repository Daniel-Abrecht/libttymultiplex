// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <curses.h>
#include <internal/pane.h>

int tym_i_csq_erase_in_line(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  if(pane->sequence.integer_count == 0)
    pane->sequence.integer[0] = 0;
//  (void)wattr_set(pane->window, 0, 0, 0);
  switch(pane->sequence.integer[0]){
    case 0: {
      wmove(pane->window, pane->cursor.y, pane->cursor.x);
      wclrtoeol(pane->window);
    } break;
    case 1: {
      for(unsigned x=0, xn=pane->cursor.x; x<xn; x++)
        mvwaddch(pane->window, pane->cursor.y, x, ' ');
    } break;
    case 2: {
      wmove(pane->window, pane->cursor.y, 0);
      wclrtoeol(pane->window);
    } break;
//    case 3: /* TODO */; break;
    default: errno = ENOSYS; return -1;
  }
  return 0;
}
