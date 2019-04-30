// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/backend.h>
#include <internal/pane.h>


int tym_i_csq_screen_alignment_test(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 0){
    errno = EINVAL;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  tym_i_backend->pane_set_area_to_character(pane, (struct tym_i_cell_position){0,0}, (struct tym_i_cell_position){.x=w,.y=h}, true, screen->character_format, 1, "E");
  return 0;
}
