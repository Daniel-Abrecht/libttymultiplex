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

enum tym_i_character_attribute {
  TYM_I_CA_DEFAULT   = 0,
  TYM_I_CA_BOLD      = 1<<0,
  TYM_I_CA_UNDERLINE = 1<<1,
  TYM_I_CA_BLINK     = 1<<2,
  TYM_I_CA_INVERSE   = 1<<3,
  TYM_I_CA_INVISIBLE = 1<<4
};

struct tym_i_character_format {
  enum tym_i_character_attribute attribute;
  struct tym_i_termcolor fgcolor;
  struct tym_i_termcolor bgcolor;
};

struct tym_i_cell_position {
  unsigned x, y;
};

union tym_i_character_data {
  struct tym_i_utf8_character_state utf8;
  char byte;
};

struct tym_i_character {
  enum charset_selection charset_selection;
  bool not_utf8;
  enum tym_i_charset_type charset_g[TYM_I_G_CHARSET_COUNT];
  union tym_i_character_data data;
};

struct tym_i_pane_screen_state {
  struct tym_i_cell_position cursor, saved_cursor;
  struct tym_i_character_format character_format;
  enum tym_i_keypad_mode keypad_mode;
  enum tym_i_cursor_key_mode cursor_key_mode;
  unsigned scroll_region_top, scroll_region_bottom;
  bool insert_mode : 1;
  bool origin_mode : 1;
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

enum tym_i_scp_scroll_region_behaviour {
  TYM_I_SCP_SCROLLING_REGION_LOCKIN_IN_ORIGIN_MODE,
  TYM_I_SCP_SCROLLING_REGION_IRRELEVANT,
  TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE,
};
enum tym_i_scp_scrolling_mode {
  TYM_I_SCP_SMM_NO_SCROLLING,
  TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY,
  TYM_I_SCP_SMM_SCROLL_BACKWARD_ONLY,
  TYM_I_SCP_SMM_UNRESTRICTED_SCROLLING,
};
enum tym_i_scp_position_mode {
  TYM_I_SCP_PM_ABSOLUTE,
  TYM_I_SCP_PM_RELATIVE,
  TYM_I_SCP_PM_ORIGIN_RELATIVE,
};
int tym_i_pane_set_cursor_position(
  struct tym_i_pane_internal* pane,
  enum tym_i_scp_position_mode pm_x, long long x,
  enum tym_i_scp_scrolling_mode smm_y, enum tym_i_scp_position_mode pm_y, long long y,
  enum tym_i_scp_scroll_region_behaviour srb, bool allow_cursor_on_right_edge
);

#endif
