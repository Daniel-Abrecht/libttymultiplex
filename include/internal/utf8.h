// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_UTF8_H
#define TYM_INTERNAL_UTF8_H

/** \file */

#include <stdint.h>

/**
 * The largest currently possible amount of bytes representing a single character.
 * This will change once more complex sequences are supported.
 */
#define TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT 4

/** The result of the addition of a byte to the utf-8 sequence in tym_i_utf8_character_state using tym_i_utf8_character_state_push.  */
enum tym_i_utf8_character_state_push_result {
  TYM_I_UCS_ERROR = -1, //!< Something went wrong, check errno. 
// states
  TYM_I_UCS_DONE = 0, //!< Sequence complete, the UTF-8 character is ready to be used.
  TYM_I_UCS_CONTINUE, //!< The current sequence is not done yet.
  TYM_I_UCS_BROKEN_IGNORE, //!< The byte of this sequence can't be the first character of the sequence. This sequence is broken, ignore it.
  TYM_I_UCS_INVALID_ABORT, //!< The passed character/byte was invalid, it can't continue the sequence. Abort this invalid sequence.
// flags
  TYM_I_UCS_INVALID_ABORT_FLAG = 1<<15, //!< The current sequence was invalidated, this character can't continue it. But the passed character may still start a valid new sequence. This flag may be set in addition to the new state of the new sequence.
// masks
  TYM_I_UCS_STATE_MASK = 0x00FF, //!< Bitmask for all states
  TYM_I_UCS_FLAG_MASK  = 0xFF00, //!< Bitmask for all flags
};

/**
 * This is a buffer that can hold a utf-8 character. This library won't deal with
 * converting every character to a number and won't use stuff like wchar_t,
 * that'll onle lead to problems in the long run.
 * Plain UTF-8 can be handled like ascii in most cases, and to store a character,
 * only the length of it needs to be determined. Updating the algorithm to determine
 * the length won't cause changes to any other parts of application logic.
 * Combined characters are not implemented yet.
 * 
 * \todo Handle combined characters as one character by updating tym_i_utf8_character_state_push.
 **/
struct tym_i_utf8_character_state {
  /** The bytes defining the character */
  uint8_t data[TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT+1];
  /** The number of bytes in data. Mustn't be bigger than TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT. */
  uint8_t count;
};

/**
 * Check if a byte completes a utf-8 character & add it if possible.
 * 
 * \param c the next byte of the utf-8 character.
 * \param state the current utf-8 character state. This is the charracter in assembly.
 * \returns The state of the utf-8 character to be assembled.
 */
enum tym_i_utf8_character_state_push_result tym_i_utf8_character_state_push(struct tym_i_utf8_character_state* state, char c);

#endif
