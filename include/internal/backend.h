// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_I_BACKEND_H
#define TYM_I_BACKEND_H

/** \file */

#include <internal/utils.h>
#include <libttymultiplex.h>
#include <stdbool.h>

/** The cursor mode */
enum tym_i_cursor_mode {
  TYM_I_CURSOR_BLOCK, //!< the cursor is a block.
  TYM_I_CURSOR_UNDERLINE, //!< The cursor is a line
  TYM_I_CURSOR_NONE, //!< The cursor is invisible
};

struct tym_i_pane_internal;
struct tym_i_cell_position;
struct tym_i_character_format;

/**
 * Optional things the backend may support
 */
struct tym_i_backend_capabilities {
  bool buffered : 1;
  bool mouse : 1;
  bool color_8 : 1;
  bool color_256 : 1;
  bool color_rgb : 1;
};

#define TYM_I_BACKEND_CALLBACKS \
  R(int, init, (struct tym_i_backend_capabilities*), (Initialise the backend.)) \
  R(int, cleanup, (bool zap), (Cleanup the backend. The backend can not be used again until it gets reinitialised. If zap is set, do not try to reset any output.)) \
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
  R(int, update_terminal_size_information, (void), (Sets #tym_i_bounds to the new size of the terminal. This is called from #tym_i_update_size_all, which shoud be called whenever the terminal/screen size changes. See #tym_i_update_size_all for all cases in which this happens automatically or should be done by the backend.)) \
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
  ), (If the application selects an alternate screen. There is only one alternate screen. Since applications usually only switch screens at startup and exit. It shouldn't matter if it isn't implemented. The new/current screen is specified by tym_i_pane_internal::current_screen. -1 should be returned if the screen can not be switched.)) \
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

/**
 * An entry for the linked list of backends.
 */
struct tym_i_backend_entry {
  /** The backend name*/
  const char* name;
  /** The backend */
  struct tym_i_backend backend;
  /** If loaded from a shared library, the dlopen handle */
  void* library;
  /** The next entry of the linked list */
  const struct tym_i_backend_entry* next;
};

bool tym_i_backend_validate_prepare(struct tym_i_backend_entry* entry);
struct tym_i_backend_entry* tym_i_backend_register(struct tym_i_backend_entry* entry);
int tym_i_backend_init(const char* backend);
void tym_i_backend_unload(bool zap);

/**
 * This is the chosen backend
 **/
extern const struct tym_i_backend* tym_i_backend;

/** Onl true if external backend is being loaded by function load_and_init_speciffic_external_backend in src/backend.c */
extern bool tym_i_external_backend_normal_loading;

/** What the backend can do. These capabillities will be passed to the init function & initialised by it. */
extern const struct tym_i_backend_capabilities* tym_i_backend_capabilities;

/**
 * Register a backend. Currently, backends are built into libttymultiplex.
 * Each backend has it's own folder. In each folder, only one backend
 * is allowed to be registred.
 * 
 * \param X The members of tym_i_backend in braces.
 **/
#define TYM_I_BACKEND_REGISTER(X) \
  static struct tym_i_backend_entry TYM_I_CONCAT(tym_i_backend_entry_,TYM_I_BACKEND_ID) = { \
    .name = TYM_I_BACKEND_NAME, \
    .backend = { TYM_I_UNPACK X } \
  }; \
  void TYM_I_CONCAT(tym_i_register_backend_,TYM_I_BACKEND_ID)(void) __attribute__((constructor(1000+TYM_I_BACKEND_PRIORITY),used)); \
  void TYM_I_CONCAT(tym_i_register_backend_,TYM_I_BACKEND_ID)(void){ \
    if(!tym_i_external_backend_normal_loading) /* If external backend loaded normally, src/backend.c takes care of this. */ \
      tym_i_backend_register(&TYM_I_CONCAT(tym_i_backend_entry_,TYM_I_BACKEND_ID)); \
  } \
  /** This weak symbol is only used if the backend is loaded as external backend. */ \
  __attribute__((weak,used,visibility("default"))) const struct tym_i_backend_entry*const tymb_backend_entry = &TYM_I_CONCAT(tym_i_backend_entry_,TYM_I_BACKEND_ID);

#endif
