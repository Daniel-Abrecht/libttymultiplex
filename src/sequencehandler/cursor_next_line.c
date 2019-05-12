// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_cursor_next_line(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  long long y = 1;
  if(pane->sequence.integer_count)
    y = pane->sequence.integer[0];
  if(y <= 0) y = 1;
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_ABSOLUTE, 0,
    TYM_I_SCP_SMM_NO_SCROLLING, TYM_I_SCP_PM_RELATIVE, y,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE, false
  );
  return 0;
}
