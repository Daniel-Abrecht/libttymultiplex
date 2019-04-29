// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_cursor_position(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 2){
    errno = ENOENT;
    return -1;
  }
  if(pane->sequence.integer_count <= 1)
    pane->sequence.integer[1] = 1;
  if(pane->sequence.integer_count == 0)
    pane->sequence.integer[0] = 1;
  unsigned y = pane->sequence.integer[0] - 1;
  unsigned x = pane->sequence.integer[1] - 1;
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_ORIGIN_RELATIVE, x,
    TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY, TYM_I_SCP_PM_ORIGIN_RELATIVE, y,
    TYM_I_SCP_SCROLLING_REGION_LOCKIN_IN_ORIGIN_MODE
  );
  return 0;
}
