// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>
#include <internal/backend.h>

int tym_i_csq_erase_characters(struct tym_i_pane_internal* pane){
  size_t n = 1;
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  if(pane->sequence.integer_count > 0)
    n = pane->sequence.integer[0];
  struct tym_i_cell_position end = {
    .x = ((unsigned long)pane->cursor.x + n) % w,
    .y = pane->cursor.y + ((unsigned long)pane->cursor.x + n) / w
  };
  tym_i_backend->pane_erase_area(pane, pane->cursor, end, false, pane->character_format);
  return 0;
}