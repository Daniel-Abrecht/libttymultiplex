// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <string.h>
#include <internal/parser.h>
#include <internal/pseudoterminal.h>

#define RSP_VT220 "62;"
#define RSP_SUPPORT_132_COLUMNS "2;"
#define RSP_SUPPORT_SELECTIVE_ERASE "6;"
#define RSP_SUPPORT_USER_DEFINED_KEYS "8;"
#define RSP_SUPPORT_NATIONAL_REPLACEMENT_CHARACTER_SETS "9;"
#define RSP_SUPPORT_TECHNICAL_CHARACTERS "15;"
#define RSP_SUPPORT_ANSI_COLOR "22;"
#define RSP_SUPPORT_ANSI_TEXT_LOCATOR "29;"

int tym_i_csq_send_device_attributes_primary(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  const char response[] = {
    CSI "?"
    RSP_VT220
    RSP_SUPPORT_SELECTIVE_ERASE
    RSP_SUPPORT_ANSI_COLOR
    "c"
  };
  tym_i_pts_send(pane, strlen(response), response);
  return 0;
}
