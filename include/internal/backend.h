// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_I_BACKEND_H
#define TYM_I_BACKEND_H

/** \file */

#include <internal/utils.h>
#include <libttymultiplex.h>
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
  R(int, init, (void), (Initialise the backend.)) \
  R(int, cleanup, (void), (Cleanup the backend. The backend can not be used again until it gets reinitialised.)) \
  R(int, resize, (void), (Handle terminal and screen size changes)) \
  R(int, pane_create, (struct tym_i_pane_internal* pane), (Backend specific initialisation of pane. The variable is for pane->backend exclusive usage by the backend.)) \
  R(void, pane_destroy, (struct tym_i_pane_internal* pane), (Backend specific cleanup of panes.)) \
  R(int, pane_resize, (struct tym_i_pane_internal* pane), (Handle resizing & repositioning of panes.)) \
  R(int, pane_scroll_region, (struct tym_i_pane_internal* pane, int n, unsigned top, unsigned bottom), (Scroll a region of the pane)) \
  R(int, pane_set_cursor_position, (struct tym_i_pane_internal* pane, struct tym_i_cell_position position), (Set the cursor position)) \
  R(int, pane_set_character, ( \
    struct tym_i_pane_internal* pane, \
    struct tym_i_cell_position position, \
    struct tym_i_character_format format, \
    size_t length, const char utf8[length+1], \
    bool insert \
  ), (Set a character at the pecified position)) \
  R(int, pane_delete_characters, \
    (struct tym_i_pane_internal* pane, struct tym_i_cell_position position, unsigned n),  \
    (Delete some characters of the pane and move the remaining ones to the left) \
  ) \
  O(int, pane_refresh, (struct tym_i_pane_internal* pane), (Refresh/redraw pane)) \
  O(int, pane_set_cursor_mode, (struct tym_i_pane_internal* pane, enum tym_i_cursor_mode cursor_mode), (Set the cursor mode. It can be a block, underlined, or invisible. )) \
  O(int, pane_scroll, (struct tym_i_pane_internal* pane, int n), (Scroll the pane.)) \
  O(int, pane_erase_area, ( \
    struct tym_i_pane_internal* pane, \
    struct tym_i_cell_position start, \
    struct tym_i_cell_position end, \
    bool block, \
    struct tym_i_character_format format \
  ), (Erase an area of the pane. If "block" is set to true, it is a rectangular region. Otherwise, the region is similar to how text is usually selected. )) \
  O(int, pane_change_screen, ( \
    struct tym_i_pane_internal* pane \
  ), (If the application selects an alternate screen. There is only one alternate screen. Since applications usually only switch screens at startup and exit. It shouldn't matter if it isn't implemented. The new/current screen is specified by tym_i_pane_internal::current_screen.)) \
  O(int, pane_set_area_to_character, ( \
    struct tym_i_pane_internal* pane, \
    struct tym_i_cell_position start, \
    struct tym_i_cell_position end, \
    bool block, \
    struct tym_i_character_format format, \
    size_t length, const char utf8[length+1] \
  ), (Similar to pane_erase_area, but sets everything to the same character.))

#define R(RET, ID, PARAMS, DOC) /** \see tym_i_backend::##ID */ typedef RET (*tym_i_ ## ID ## _proc) PARAMS;
#define O(RET, ID, PARAMS, DOC) R(RET, ID, PARAMS, DOC) /** \see tym_i_backend::##ID */ RET tym_i_ ## ID ## _default_proc PARAMS;
TYM_I_BACKEND_CALLBACKS
#undef R
#undef O

/**
 * This is the backend interface. A backend has to implement all required methods.
 * Backend are currently registred using the TYM_I_BACKEND_REGISTER macro.
 **/
struct tym_i_backend {
  #define R(RET, ID, PARAMS, DOC) TYM_I_DOKUMENTATION DOC tym_i_ ## ID ## _proc ID;
  #define O(RET, ID, PARAMS, DOC) /** Optional, the default implementation is #tym_i_ ## ID ## _default_proc. <br/><br/> */ TYM_I_DOKUMENTATION DOC tym_i_ ## ID ## _proc ID;
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

/**
 * This is the chowsen backend
 **/
extern const struct tym_i_backend* tym_i_backend;

/**
 * Register a backend. Currently, backends are built into libttymultiplex.
 * TODO: Put backends into shared libraries instead.
 * 
 * \param P This is the priority determining the backend order.
 * \param N The backend name specified as a c string constant.
 * \param X The members of tym_i_backend in braces.
 **/
#define TYM_I_BACKEND_REGISTER(P,N,X) \
  static void TYM_I_LUNIQUE(register_backend)(void) __attribute__((constructor(P),used)); \
  static void TYM_I_LUNIQUE(register_backend)(void){ \
    static struct tym_i_backend_entry entry = { \
      .name = N, \
      .backend = { TYM_I_UNPACK X } \
    }; \
    tym_i_backend_register(&entry); \
  }

#endif
