// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_reset_mode(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 1){
    errno = ENOENT;
    return -1;
  }
  int c = pane->sequence.integer[0];
  switch(c){
    case 2: ; break; // Keyboard Action Mode (AM)
    case 4: ; break; // Replace Mode (IRM)
    case 12: ; break; // Send/receive (SRM)
    case 20: ; break; // Normal Linefeed (LNM)
    default: errno = -ENOSYS; break;
  }
  return 0;
}
