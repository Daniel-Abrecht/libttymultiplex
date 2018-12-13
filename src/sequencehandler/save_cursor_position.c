// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_save_cursor_position(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 0){
    errno = ENOENT;
    return -1;
  }
  pane->saved_cursor = pane->cursor;
  return 0;
}
