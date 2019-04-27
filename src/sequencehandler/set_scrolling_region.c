// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_set_scrolling_region(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 2){
    errno = ENOENT;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned top = 0;
  unsigned bottom = ~0u;
  if(pane->sequence.integer_count >= 1){
    top = pane->sequence.integer[0] - 1;
    if(!pane->sequence.integer[0])
      top = 0;
  }
  if(pane->sequence.integer_count >= 2)
    bottom = pane->sequence.integer[1];
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  if(bottom > h)
    bottom = h;
  if(top == 0 && bottom == h)
    bottom = 0;
  if(top >= bottom){
    top = 0;
    bottom = 0;
  }
  screen->scroll_region_top    = top;
  screen->scroll_region_bottom = bottom;
  tym_i_pane_cursor_set_cursor(pane, 0, 0, TYM_I_SMB_ORIGIN_MODE);
  return 0;
}
