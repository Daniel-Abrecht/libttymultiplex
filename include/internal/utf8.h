// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_UTF8_H
#define TYM_INTERNAL_UTF8_H

#include <stdint.h>

enum {
  TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT = 4
};

enum tym_i_utf8_character_state_push_result {
  TYM_I_UCS_ERROR = -1,
// states
  TYM_I_UCS_DONE = 0, // sequence complete
  TYM_I_UCS_CONTINUE, // current sequence not done yet
  TYM_I_UCS_BROKEN_IGNORE, // Continued sequence without start, ignore
  TYM_I_UCS_INVALID_ABORT, // The passed character was invalid
// flags
  TYM_I_UCS_INVALID_ABORT_FLAG = 1<<15, // The current sequence was invalidated, but the passed character may still start a valid new sequence
// masks
  TYM_I_UCS_STATE_MASK = 0x00FF,
  TYM_I_UCS_FLAG_MASK  = 0xFF00,
};

// TODO: handle stuff like combined characters as one character
struct tym_i_utf8_character_state {
  uint8_t data[TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT+1];
  uint8_t count;
};

enum tym_i_utf8_character_state_push_result tym_i_utf8_character_state_push(struct tym_i_utf8_character_state* state, char c);

#endif
