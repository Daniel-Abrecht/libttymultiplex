// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>
#include <internal/backend.h>

int tym_i_csq_reset(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 0){
    errno = ENOENT;
    return -1;
  }
  memset(pane->screen, 0, sizeof(pane->screen));
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  pane->mouse_mode = MOUSE_MODE_OFF;
  pane->character.not_utf8 = false;
  tym_i_backend->pane_erase_area(pane, (struct tym_i_cell_position){.x=0,.y=0}, (struct tym_i_cell_position){.x=w,.y=h}, false, screen->character_format);
  tym_i_pane_cursor_set_cursor(pane, 0, 0);
  return 0;
}
