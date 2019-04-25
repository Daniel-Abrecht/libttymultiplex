// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

// TODO: actually implement this mode

int tym_i_csq_application_keypad(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 0){
    errno = ENOENT;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  screen->keypad_mode = TYM_I_KEYPAD_MODE_APPLICATION;
  return 0;
}
