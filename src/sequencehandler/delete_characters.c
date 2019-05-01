// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/backend.h>
#include <internal/pane.h>

int tym_i_csq_delete_characters(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  unsigned n = 1;
  if(pane->sequence.integer_count)
    n = pane->sequence.integer[0];
  if(n == 0) n = 1;
  tym_i_backend->pane_delete_characters(pane, pane->screen[pane->current_screen].cursor, n);
  return 0;
}
