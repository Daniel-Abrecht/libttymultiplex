// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <curses.h>
#include <internal/pane.h>

int tym_i_csq_erase_in_display(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  if(pane->sequence.integer_count == 0)
    pane->sequence.integer[0] = 0;
  switch(pane->sequence.integer[0]){
    case 0: wmove(pane->window, pane->cursor.y, pane->cursor.x); wclrtobot(pane->window); break;
    case 1: {
      for(unsigned y=0, yn=pane->cursor.y; y<yn; y++){
        wmove(pane->window, y, 0);
        wclrtoeol(pane->window);
      }
    } break;
    case 2: wclear(pane->window); break;
//    case 3: /* TODO */; break;
    default: errno = ENOSYS; return -1;
  }
  return 0;
}
