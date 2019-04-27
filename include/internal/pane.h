// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_PANE_H
#define TYM_INTERNAL_PANE_H

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

enum tym_i_pane_screen {
  TYM_I_SCREEN_DEFAULT,
  TYM_I_SCREEN_ALTERNATE,
  TYM_I_SCREEN_COUNT
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
/*
  TYM_I_DSDR_AUTO_WRAP_MODE = 7,
  TYM_I_DSDR_AUTO_REPEAT_KEYS = 8,*/
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

enum tym_i_button {
  TYM_I_BUTTON_LEFT_PRESSED,
  TYM_I_BUTTON_MIDDLE_PRESSED,
  TYM_I_BUTTON_RIGHT_PRESSED,
  TYM_I_BUTTON_RELEASED,
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
};

struct tym_i_pane_internal {
  struct tym_i_pane_internal *previous, *next;
  int id;
  int master, slave;
  bool nofocus;
  void* backend;
  struct tym_i_sequence_state sequence;
  struct termios termios;
  struct tym_superposition superposition;
  struct tym_absolute_position coordinates;
  size_t resize_handler_count;
  struct tym_i_handler_ptr_pair* resize_handler_list;

  struct tym_i_character character;
  enum tym_i_pane_screen current_screen;
  struct tym_i_cell_position last_mouse_event_pos;
  enum tym_i_button last_button;
  enum mouse_mode mouse_mode;

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
int tym_i_scroll_scrolling_region(struct tym_i_pane_internal* pane, int n);
void tym_i_perror(const char*);

enum set_cursor_scrolling_mode_behaviour {
  TYM_I_SMB_NORMAL,
  TYM_I_SMB_IGNORE,
  TYM_I_SMB_ORIGIN_MODE
};
int tym_i_pane_cursor_set_cursor(struct tym_i_pane_internal* pane, unsigned x, unsigned y, enum set_cursor_scrolling_mode_behaviour);

#endif
