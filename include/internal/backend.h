// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_I_BACKEND_H
#define TYM_I_BACKEND_H

#include <internal/utils.h>
#include <stdbool.h>

enum tym_i_cursor_mode {
  TYM_I_CURSOR_BLOCK,
  TYM_I_CURSOR_UNDERLINE,
  TYM_I_CURSOR_NONE,
};

struct tym_i_pane_internal;
struct tym_i_cell_position;
struct tym_i_character_format;

#define TYM_I_BACKEND_CALLBACKS \
  R(int, init, (void)) \
  R(int, cleanup, (void)) \
  R(int, resize, (void)) \
  R(int, pane_create, (struct tym_i_pane_internal* pane)) \
  R(void, pane_destroy, (struct tym_i_pane_internal* pane)) \
  R(int, pane_resize, (struct tym_i_pane_internal* pane)) \
  R(int, pane_scroll_region, (struct tym_i_pane_internal* pane, int n, unsigned top, unsigned bottom)) \
  R(int, pane_set_cursor_position, (struct tym_i_pane_internal* pane, struct tym_i_cell_position position)) \
  R(int, pane_set_character, ( \
    struct tym_i_pane_internal* pane, \
    struct tym_i_cell_position position, \
    struct tym_i_character_format format, \
    size_t length, const char utf8[length+1], \
    bool insert \
  )) \
  R(int, pane_delete_characters, (struct tym_i_pane_internal* pane, struct tym_i_cell_position position, unsigned n)) \
  O(int, pane_refresh, (struct tym_i_pane_internal* pane)) \
  O(int, pane_set_cursor_mode, (struct tym_i_pane_internal* pane, enum tym_i_cursor_mode cursor_mode)) \
  O(int, pane_scroll, (struct tym_i_pane_internal* pane, int n)) \
  O(int, pane_erase_area, ( \
    struct tym_i_pane_internal* pane, \
    struct tym_i_cell_position start, \
    struct tym_i_cell_position end, \
    bool block, \
    struct tym_i_character_format format \
  )) \
  O(int, pane_change_screen, ( \
    struct tym_i_pane_internal* pane \
  )) \
  O(int, pane_set_area_to_character, ( \
    struct tym_i_pane_internal* pane, \
    struct tym_i_cell_position start, \
    struct tym_i_cell_position end, \
    bool block, \
    struct tym_i_character_format format, \
    size_t length, const char utf8[length+1] \
  ))

#define R(RET, ID, PARAMS) typedef RET (*tym_i_ ## ID ## _proc) PARAMS;
#define O(RET, ID, PARAMS) R(RET, ID, PARAMS) RET tym_i_ ## ID ## _default_proc PARAMS;
TYM_I_BACKEND_CALLBACKS
#undef R
#undef O

struct tym_i_backend {
  #define R(RET, ID, PARAMS) tym_i_ ## ID ## _proc ID;
  #define O(RET, ID, PARAMS) R(RET, ID, PARAMS)
  TYM_I_BACKEND_CALLBACKS
  #undef R
  #undef O
};

struct tym_i_backend_entry {
  const char* name;
  struct tym_i_backend backend;
  const struct tym_i_backend_entry* next;
};

void tym_i_backend_register(struct tym_i_backend_entry* entry);
int tym_i_backend_init(const char* backend);

extern const struct tym_i_backend* tym_i_backend;

#define TYM_I_BACKEND_REGISTER(P,N,X) \
  static void TYM_LUNIQUE(register_backend)(void) __attribute__((constructor(P),used)); \
  static void TYM_LUNIQUE(register_backend)(void){ \
    static struct tym_i_backend_entry entry = { \
      .name = N, \
      .backend = { TYM_UNPACK X } \
    }; \
    tym_i_backend_register(&entry); \
  }

#endif
