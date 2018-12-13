// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <internal/list.h>

enum { LIST_BLOCK_SIZE = 32 };

int tym_i_list_add(size_t s, size_t*restrict pn, void*restrict*restrict ptr, const void*restrict entry){
  size_t n = *pn;
  size_t m = (n + LIST_BLOCK_SIZE - 1) / LIST_BLOCK_SIZE * LIST_BLOCK_SIZE;
  size_t l = (n + LIST_BLOCK_SIZE    ) / LIST_BLOCK_SIZE * LIST_BLOCK_SIZE;
  if(l != m){
    void* next = realloc(*ptr, l * s);
    if(!next)
      return -1;
    memset((char*)next + n * s, 0, s * (l - n));
    *ptr = next;
  }
  memcpy((char*)(*ptr) + n*s, entry, s);
  *pn = n + 1;
  return 0;
}

int tym_i_list_remove(size_t s, size_t*restrict pn, void*restrict*restrict ptr, size_t entry){
  size_t n = *pn;
  if(entry >= n){
    errno = EINVAL;
    return -1;
  }
  if(n-1 != entry)
    memmove((char*)(*ptr)+(entry+1)*s, (char*)(*ptr)+(entry)*s, (n-entry-1) * s);
  n -= 1;
  if(!n){
    free(*ptr);
    *pn = 0;
    *ptr = 0;
    return 0;
  }
  *pn = n;
  size_t m = (n + LIST_BLOCK_SIZE    ) / LIST_BLOCK_SIZE * LIST_BLOCK_SIZE;
  size_t l = (n + LIST_BLOCK_SIZE + 1) / LIST_BLOCK_SIZE * LIST_BLOCK_SIZE;
  if(m != l){
    void* result = realloc(*ptr, s * m);
    if(result) *ptr = result;
  }
  return 0;
}

