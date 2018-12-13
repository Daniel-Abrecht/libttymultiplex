// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <string.h>
#include <internal/parser.h>
#include <internal/pseudoterminal.h>

enum {
  STATUS_REPORT = 5,
  CURSOR_POSITION = 6
};

#define S(X) strlen(X), X

int tym_i_csq_device_status_report(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count != 1){
    errno = ENOENT;
    return -1;
  }
  char buffer[64];
  switch(pane->sequence.integer[0]){
    case STATUS_REPORT  : tym_i_pts_send(pane, S(CSI "0n")); break; // OK
    case CURSOR_POSITION: {
      snprintf(buffer, sizeof(buffer), CSI "%u;%uR", pane->cursor.y+1, pane->cursor.x+1);
      tym_i_pts_send(pane, S(buffer));
    } break;
  }
  return 0;
}
