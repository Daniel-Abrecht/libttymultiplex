// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_PANE_H
#define TYM_INTERNAL_PANE_H

/** \file */

#include <poll.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <internal/utf8.h>
#include <internal/charset.h>
#include <libttymultiplex.h>

struct tym_i_handler_ptr_pair {
  void* ptr;
  tym_resize_handler_t callback;
};

/** The selected screen. */
enum tym_i_pane_screen {
  TYM_I_SCREEN_DEFAULT /** 0 */,
  TYM_I_SCREEN_ALTERNATE /** 1 */,
  TYM_I_SCREEN_COUNT /** 2 */
};

enum {
  TYM_I_MAX_SEQ_LEN = 256,
  TYM_I_MAX_INT_COUNT = 12,
};

struct tym_i_sequence_state {
  unsigned short length;
  unsigned short index;
  bool last_special_match_continue;
  char buffer[TYM_I_MAX_SEQ_LEN];
  unsigned integer_count;
  int integer[TYM_I_MAX_INT_COUNT];
  ssize_t seq_opt_min, seq_opt_max;
};

enum { TYM_I_G_CHARSET_COUNT=4 };

enum tym_i_keypad_mode {
  TYM_I_KEYPAD_MODE_NORMAL, // (numeric keypad mode)
  TYM_I_KEYPAD_MODE_APPLICATION,
};

enum tym_i_cursor_key_mode {
  TYM_I_CURSOR_KEY_MODE_NORMAL,
  TYM_I_CURSOR_KEY_MODE_APPLICATION,
};

enum mouse_mode {
  MOUSE_MODE_OFF,
  MOUSE_MODE_X10,
  MOUSE_MODE_BUTTON,
  MOUSE_MODE_NORMAL, // VT220
  MOUSE_MODE_ANY
};

enum tym_i_setmode {
  TYM_I_SM_KEYBOARD_ACTION,
  TYM_I_SM_INSERT,
  TYM_I_SM_SEND_RECEIVE,
  TYM_I_SM_AUTOMATIC_NEWLINE
};

enum tym_i_decset_decres {
  TYM_I_DSDR_APPLICATION_CURSOR_KEYS = 1,
/*  TYM_I_DSDR_DESIGNATE_USASCII_CHARACTER_SETS = 2,
  TYM_I_DSDR_132_COLUMN_MODE = 3,
  TYM_I_DSDR_SMOOTH_SCROLL = 4,
  TYM_I_DSDR_REVERSE_VIDEO = 5,
*/
  TYM_I_DSDR_ORIGIN_MODE = 6,
  TYM_I_DSDR_AUTO_WRAP_MODE = 7,
/*  TYM_I_DSDR_AUTO_REPEAT_KEYS = 8,*/
  TYM_I_DSDR_MOUSE_MODE_X10 = 9,
  TYM_I_DSDR_ALTERNATE_SCREEN_1 = 47,
  TYM_I_DSDR_APPLICATION_KEYPAD = 66,
  // TODO: fill in the rest...
  TYM_I_DSDR_MOUSE_MODE_NORMAL = 1000,
  TYM_I_DSDR_MOUSE_MODE_BUTTON = 1002,
  TYM_I_DSDR_MOUSE_MODE_ANY = 1003,
  TYM_I_DSDR_ALTERNATE_SCREEN_2 = 1047,
  TYM_I_DSDR_ALTERNATE_SCREEN_3 = 1049,
};

struct tym_i_termcolor {
  unsigned char index, red, green, blue;
};

/** Character formate attributes. Multiple are possible. */
enum tym_i_character_attribute {
  TYM_I_CA_DEFAULT   = 0,    //!< The default, no sepcial formatting
  TYM_I_CA_BOLD      = 1<<0, //!< Bold text
  TYM_I_CA_UNDERLINE = 1<<1, //!< Underlined text
  TYM_I_CA_BLINK     = 1<<2, //!< Blinking text
  TYM_I_CA_INVERSE   = 1<<3, //!< Switch foreground and background color
  TYM_I_CA_INVISIBLE = 1<<4  //!< The text isn't visible at all
};

/** The format of the current character */
struct tym_i_character_format {
  /** Character attribute */
  enum tym_i_character_attribute attribute;
  /** The foreground color */
  struct tym_i_termcolor fgcolor;
  /** The background color */
  struct tym_i_termcolor bgcolor;
};

/** A position specified in characters */
struct tym_i_cell_position {
  /** The X position */
  unsigned x;
  /** The Y position */
  unsigned y;
};

/** The character data of tym_i_character */
union tym_i_character_data {
  /** In case of a utf-8 character. If it is one can be checked using tym_i_character_is_utf8. */
  struct tym_i_utf8_character_state utf8;
  /**
   * The character code in case of a non-utf8 charset.
   * It'll be converted to utf-8 before usage by considering tym_i_character::charset_g and tym_i_translation_table though.
   */
  char byte;
};

/** Parser state of the current character. */
struct tym_i_character {
  /** The selected charset designation from charset_g. */
  enum charset_selection charset_selection;
  /** If the character is not utf-8. The opposit isn't always true, use tym_i_character_is_utf8 for checking if it's a utf-8 character. */
  bool not_utf8;
  /** The selected character sets. \see charset_selection */
  enum tym_i_charset_type charset_g[TYM_I_G_CHARSET_COUNT];
  /** The actual character. */
  union tym_i_character_data data;
};

/**
 * These are pane states which apply on a per-screen basis rather than a per-pane basis.
 * \see tym_i_pane_internal::screen
 * \see tym_i_pane_internal::current_screen
 */
struct tym_i_pane_screen_state {
  /** The current cursor position */
  struct tym_i_cell_position cursor;
  /** The last cursor sequence saved by an escape sequence, can be restored later. */
  struct tym_i_cell_position saved_cursor;
  /** The current formatting for the characters that will be written next. */
  struct tym_i_character_format character_format;
  /** This doesn't actually do anything at the moment */
  enum tym_i_keypad_mode keypad_mode;
  /** This affects the escape sequence which has to be sent for some keys. This is handled in pseudoterminal.h. */
  enum tym_i_cursor_key_mode cursor_key_mode;
  /** The top of the scrolling region. */
  unsigned scroll_region_top;
  /** The bottom of the scrolling region. */
  unsigned scroll_region_bottom;
  /** A flag indicating wheter the insert mode is active */
  bool insert_mode : 1;
  /** A flag indicating wheter the origin mode is active. \see tym_i_pane_set_cursor_position */
  bool origin_mode : 1;
  /** Indicates wheter at one character after the end of a line, after a character is input, the cursor should not be put on the next line before writing it. */
  bool wraparound_mode_off : 1;
};

/** Internal variables of a pane */
struct tym_i_pane_internal {
  /** Doubly linked list, previous entry */
  struct tym_i_pane_internal *previous;
  /** Doubly linked list, next entry */
  struct tym_i_pane_internal *next;
  /**
   * The unique pane id. Some number bigger than 1.
   * 1 is reserved for the pane in focus, see #TYM_PANE_FOCUS.
   * The pane in focus doesn't have the id 1 though, the id never changes
   */
  int id;
  /** The pseudo terminal master (PTM) file descriptor */
  int master;
  /** The pseudo terminal slave (PTS) file descriptor */
  int slave;
  /** A flag indicating if fetting the focus on this pane is disallowed */
  bool nofocus;
  /** A private variable reserved for usage by the backend */
  void* backend;
  /** The current state of the escape sequence parser. */
  struct tym_i_sequence_state sequence;
  /** The initial settings of the pseudo terminal */
  struct termios termios;
  /** The user specified position of the pane */
  struct tym_super_position_rectangle super_position;
  /** The computed position of the pane */
  struct tym_absolute_position_rectangle absolute_position;
  /** The number of registred resize handlers */
  size_t resize_handler_count;
  /** A list of registred resize handlers */
  struct tym_i_handler_ptr_pair* resize_handler_list;
  /** The parser state of the current character */
  struct tym_i_character character;
  /** The current screen */
  enum tym_i_pane_screen current_screen;
  /** The position of the mouse during the last event. This is used to check wheter it changed for the next event. */
  struct tym_i_cell_position last_mouse_event_pos;
  /** The mouse button state of the last event. This is used to check wheter it changed for the next event. */
  enum tym_button last_button;
  /** The current mouse mode. Specifies what kind of mouse events are sent and how. */
  enum mouse_mode mouse_mode;

  /** These are states which apply on a per-screen basis rather than a per-pane basis. */
  struct tym_i_pane_screen_state screen[TYM_I_SCREEN_COUNT];
};

extern struct tym_i_pane_internal *tym_i_pane_list_start, *tym_i_pane_list_end;
extern struct tym_i_pane_internal *tym_i_focus_pane;
extern const struct tym_i_character_format default_character_format;

bool tym_i_character_is_utf8(struct tym_i_character character);
void tym_i_pane_update_size_all(void);
void tym_i_pane_update_size(struct tym_i_pane_internal* pane);
int tym_i_pane_resize_handler_add(struct tym_i_pane_internal* pane, const struct tym_i_handler_ptr_pair* cp);
int tym_i_pane_resize_handler_remove(struct tym_i_pane_internal* pane, size_t entry);
void tym_i_pane_add(struct tym_i_pane_internal* pane);
struct tym_i_pane_internal* tym_i_pane_get(int pane);
void tym_i_pane_parse(struct tym_i_pane_internal* pane, unsigned char c);
void tym_i_pane_remove(struct tym_i_pane_internal* pane);
int tym_i_pane_focus(struct tym_i_pane_internal* pane);
void tym_i_pane_update_cursor(struct tym_i_pane_internal* pane);
int tym_i_pane_set_screen(struct tym_i_pane_internal* pane, enum tym_i_pane_screen screen);
int tym_i_scroll_def_scrolling_region(struct tym_i_pane_internal* pane, unsigned top, unsigned bottom, int n);
int tym_i_scroll_scrolling_region(struct tym_i_pane_internal* pane, int n);
int tym_i_pane_insert_delete_lines(struct tym_i_pane_internal* pane, unsigned y, int n);
void tym_i_perror(const char*);
int tym_i_pane_reset(struct tym_i_pane_internal* pane);

/** How the coordinate change is affected by the scrolling region. */
enum tym_i_scp_scroll_region_behaviour {
  TYM_I_SCP_SCROLLING_REGION_LOCKIN_IN_ORIGIN_MODE, //!< If origin mode is active, the cursor can't be placed outside the scrolling region.
  TYM_I_SCP_SCROLLING_REGION_IRRELEVANT, //!< The scrolling region doesn't matter at all
  TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE, //!< The cursor can't go from inside to outside the scrolling region
};
/** Is scrolling possible */
enum tym_i_scp_scrolling_mode {
  TYM_I_SCP_SMM_NO_SCROLLING, //!< Never scroll
  TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY, //!< It's only possible to scroll forwards (down/right)
  TYM_I_SCP_SMM_SCROLL_BACKWARD_ONLY, //!< It's only possible to scroll backwards (up/left) 
  TYM_I_SCP_SMM_UNRESTRICTED_SCROLLING, //!< Allow scrolling in any direction
};
/** What the specified position is relative to */
enum tym_i_scp_position_mode {
  TYM_I_SCP_PM_ABSOLUTE, //!< The cursor position is relative to the pane
  TYM_I_SCP_PM_RELATIVE, //!< The cursor position is relative to the current cursor position
  TYM_I_SCP_PM_ORIGIN_RELATIVE, //!< The cursor position is relative to the scrolling region if it is valid and origin mode is set. Otherwise, it's relative to the pane
};
int tym_i_pane_set_cursor_position(
  struct tym_i_pane_internal* pane,
  enum tym_i_scp_position_mode pm_x, long long x,
  enum tym_i_scp_scrolling_mode smm_y, enum tym_i_scp_position_mode pm_y, long long y,
  enum tym_i_scp_scroll_region_behaviour srb, bool allow_cursor_on_right_edge
);

#endif
