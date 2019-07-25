// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_repeat_preceding_character(struct tym_i_pane_internal* pane){
  if(pane->sequence.integer_count > 1){
    errno = ENOENT;
    return -1;
  }
  long long n = 1;
  if(pane->sequence.integer_count)
    n = pane->sequence.integer[0];
  if(n == 0)
    n = 1;
  while(n--)
    tym_i_print_character(pane, pane->last_character);
  return 0;
}
