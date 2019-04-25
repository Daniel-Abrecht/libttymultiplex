// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef LIBTTYMULTIPLEX_H
#define LIBTTYMULTIPLEX_H

#include <stdbool.h>
#include <stddef.h>

#ifndef TYM_EXPORT
#ifdef TYM_BUILD
#define TYM_EXPORT __attribute__((visibility ("default")))
#else
#define TYM_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum tym_special_key {
  TYM_KEY_ENTER,
  TYM_KEY_UP,
  TYM_KEY_DOWN,
  TYM_KEY_RIGHT,
  TYM_KEY_LEFT,
  TYM_KEY_BACKSPACE
};

enum tym_position_type {
  TYM_P_UNSET,
  TYM_P_CHARFIELD,
  TYM_P_RATIO,
  TYM_P_COUNT
};

enum tym_unit_type {
  TYM_U_UNSET,
  TYM_U_INTEGER,
  TYM_U_REAL,
  TYM_U_COUNT
};

enum tym_flag {
  TYM_PF_FOCUS,
  TYM_PF_DISALLOW_FOCUS
};

struct tym_unit {
  enum tym_unit_type type;
  union {
    int integer;
    double real;
  } value;
};

struct tym_position {
  enum tym_position_type type;
  struct tym_unit axis[2];
};

struct tym_superposition {
  struct tym_position position[TYM_P_COUNT][2];
};

struct tym_absolute_position {
  struct tym_position position[TYM_P_COUNT][2];
};

typedef void(*tym_resize_handler_t)(void* ptr, int pane, const struct tym_superposition* input, const struct tym_absolute_position* computed );

extern enum tym_unit_type tym_positon_unit_map[];

TYM_EXPORT int tym_init(void);
TYM_EXPORT int tym_shutdown(void);
TYM_EXPORT int tym_pane_create(const struct tym_superposition*restrict superposition);
TYM_EXPORT int tym_pane_destroy(int pane);
TYM_EXPORT int tym_pane_resize(int pane, const struct tym_superposition*restrict superposition);
TYM_EXPORT int tym_pane_reset(int pane);
TYM_EXPORT int tym_register_resize_handler( int pane, void* ptr, tym_resize_handler_t handler );
TYM_EXPORT int tym_unregister_resize_handler( int pane, void* ptr, tym_resize_handler_t handler );
TYM_EXPORT int tym_pane_set_flag(int pane, enum tym_flag flag, bool status);
TYM_EXPORT int tym_pane_get_flag(int pane, enum tym_flag flag);
TYM_EXPORT int tym_pane_set_env(int pane);
TYM_EXPORT int tym_pane_get_slavefd(int pane);
TYM_EXPORT int tym_pane_get_masterfd(int pane);

#ifdef __cplusplus
}
#endif

#endif
