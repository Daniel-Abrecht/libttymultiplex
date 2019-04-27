// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_scroll_down(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 1){
    errno = ENOENT;
    return -1;
  }
  unsigned y = 1;
  if(pane->sequence.integer_count)
    y = pane->sequence.integer[0];
  tym_i_scroll_scrolling_region(pane, -y);
  return 0;
}
