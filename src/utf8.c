// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <string.h>
#include <internal/utf8.h>

/** \file */

static int utf8_length(uint8_t b){
  if( (b & 0x80) == 0 )
    return 1;
  if( (b & 0xE0) == 0xC0 )
    return 2;
  if( (b & 0xF0) == 0xE0 )
    return 3;
  if( (b & 0xF8) == 0xF0 )
    return 4;
  return -1;
}

enum tym_i_utf8_character_state_push_result tym_i_utf8_character_state_push(struct tym_i_utf8_character_state* state, char c){
  enum tym_i_utf8_character_state_push_result ret = 0;
  enum tym_i_utf8_character_state_push_result ret_flags = 0;
  uint8_t b = c;
  if(!state){
    errno = EINVAL;
    return TYM_I_UCS_ERROR;
  }
  int n = utf8_length(state->data[0]);
  if( state->count > TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT || n <= 0 || state->count >= n ){
    memset(state,0,sizeof(*state));
    n = 0;
  }
  if( state->count ){
    if( (b & 0xC0) == 0x80 ){
      state->data[state->count++] = b;
      ret = state->count == n ? TYM_I_UCS_DONE : TYM_I_UCS_CONTINUE;
      goto end;
    }
    ret_flags |= TYM_I_UCS_INVALID_ABORT_FLAG;
    memset(state,0,sizeof(*state));
    n = 0;
  }
  if( (b & 0x80) == 0 ){
    ret = TYM_I_UCS_DONE;
  }else if( (b & 0xC0) == 0x80 ){
    ret = TYM_I_UCS_BROKEN_IGNORE;
  }else if( (b & 0xE0) == 0xC0 || (b & 0xF0) == 0xE0 || (b & 0xF8) == 0xF0 ){
    ret = TYM_I_UCS_CONTINUE;
  }else{
    ret = TYM_I_UCS_INVALID_ABORT;
    goto end;
  }
  state->count = 1;
  state->data[0] = b;
end:
  return ret | ret_flags;
}
