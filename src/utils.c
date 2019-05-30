// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <stdlib.h>
#include <string.h>
#include <internal/utils.h>

/** \file */

void* tym_i_copy(size_t s, void* init){
  void* ret = calloc(1, s);
  if(!ret) return 0;
  if(init) memcpy(ret, init, s);
  return ret;
}
