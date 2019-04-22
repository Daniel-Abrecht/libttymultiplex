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
  if(pane->sequence.integer_count >= 1)
    top = pane->sequence.integer[0] - 1;
  if(pane->sequence.integer_count >= 2)
    bottom = pane->sequence.integer[0] - 1;
  screen->scroll_region_top    = top;
  screen->scroll_region_bottom = bottom;
  return 0;
}
