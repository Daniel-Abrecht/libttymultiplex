// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_reverse_index(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 0){
    errno = ENOENT;
    return -1;
  }
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_RELATIVE, 0,
    TYM_I_SCP_SMM_SCROLL_BACKWARD_ONLY, TYM_I_SCP_PM_RELATIVE, -1,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE
  );
  return 0;
}
