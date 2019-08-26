// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_PARSER_H
#define TYM_INTERNAL_PARSER_H

#include <internal/charset.h>

/** \file */

#define ESC "\x1B"
#define IND ESC "D"
#define HEL ESC "E"
#define HTS ESC "H"
#define RI  ESC "M"
#define SS2 ESC "N"
#define SS3 ESC "O"
#define DCS ESC "P"
#define SPA ESC "V"
#define EPA ESC "W"
#define SOS ESC "X"
#define DECID ESC "Z"
#define CSI ESC "["
#define ST  ESC "\\"
#define OSC ESC "]"
#define PM  ESC "^"
#define APC ESC "_"
#define RIS ESC "c"

#define C  "\1"
#define NUM "\2"
#define SNUM "\3"
#define TEXT "\4"

struct tym_i_pane_internal;

/**
 * This is the type of the callback functions which are called if an escape sequence matches.
 * 
 * \see tym_i_pane_internal::sequence
 */
typedef int (*tym_i_csq_sequence_callback)(struct tym_i_pane_internal* pane);

/**
 * This is the mapping between escape sequences, their description/name and their callback function.
 */
struct tym_i_command_sequence {
  /** The escape sequence template */
  const char* sequence;
  /** The length of the escape sequence template */
  unsigned short length;
  /** The name of the callback function */
  const char* callback_name;
  /** A pointer to the callback function */
  tym_i_csq_sequence_callback callback;
};

/**
 * This is just a hook for some white box tests to check if an escape sequence
 * was detected
 */
extern void tym_i_csq_test_hook(const struct tym_i_pane_internal* pane, int ret, const struct tym_i_command_sequence* command) __attribute__((weak));

/**
 * This is just a hook for some white box tests to check if a character was
 * detected as not to be part of an escape sequence
 */
extern void tym_i_nocsq_test_hook(const struct tym_i_pane_internal* pane, char c) __attribute__((weak));

/**
 * The parser function for parsing escape sequences.
 * The main loop will pass every character it has read from the pseudo terminal master
 * of a pane to this function, one byte at a time.
 * The parser state is stored on a per-pane basis.
 */
void tym_i_pane_parse(struct tym_i_pane_internal* pane, unsigned char c);

int tym_i_invoke_charset(struct tym_i_pane_internal* pane, enum charset_selection cs);

#endif
