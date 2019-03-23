// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>
#include <internal/backend.h>

int tym_i_csq_erase_in_display(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  if(pane->sequence.integer_count == 0)
    pane->sequence.integer[0] = 0;
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  switch(pane->sequence.integer[0]){
    case 0: tym_i_backend->pane_erase_area(pane, pane->cursor, (struct tym_i_cell_position){.x=w,.y=h}, false, pane->character_format); break;
    case 1: tym_i_backend->pane_erase_area(pane, (struct tym_i_cell_position){.x=0,.y=0}, pane->cursor, false, pane->character_format); break;
    case 2: tym_i_backend->pane_erase_area(pane, (struct tym_i_cell_position){.x=0,.y=0}, (struct tym_i_cell_position){.x=w,.y=h}, false, pane->character_format); break;
//    case 3: /* TODO */; break;
    default: errno = ENOSYS; return -1;
  }
  return 0;
}
