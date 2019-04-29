// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_cursor_right(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  long long x = 1;
  if(pane->sequence.integer_count)
    x = pane->sequence.integer[0];
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_RELATIVE, x,
    TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY, TYM_I_SCP_PM_RELATIVE, 0,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE
  );
  return 0;
}
