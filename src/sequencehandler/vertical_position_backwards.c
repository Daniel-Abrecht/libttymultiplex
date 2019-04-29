// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_vertical_position_backwards(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  long long y = 1;
  if(pane->sequence.integer_count)
    y = pane->sequence.integer[0];
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_RELATIVE, 0,
    TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY, TYM_I_SCP_PM_RELATIVE, -y,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE
  );
  return 0;
}
