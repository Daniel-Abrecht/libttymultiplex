// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef LIBTTYMULTIPLEX_H
#define LIBTTYMULTIPLEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef TYM_EXPORT
#ifdef TYM_BUILD
#define TYM_EXPORT __attribute__((visibility ("default")))
#else
#define TYM_EXPORT
#endif
#endif

#define TYM_CONCAT_SUB(A,B) A ## B
#define TYM_CONCAT(A,B) TYM_CONCAT_SUB(A,B)

#ifdef __cplusplus
extern "C" {
#endif

#define TYM_SPECIAL_KEYS \
  X(ENTER    , '\n'  ) \
  X(HOME     , '\r'  ) \
  X(BACKSPACE, '\b'  ) \
  X(TAB      , '\t'  ) \
  X(DELETE   , '\x7F') \
  X(UP       , 0x100 ) \
  X(DOWN     , 0x101 ) \
  X(RIGHT    , 0x102 ) \
  X(LEFT     , 0x103 ) \
  X(END      , 0x104 )

enum tym_special_key {
#define X(ID, VAL) TYM_KEY_ ## ID = VAL,
TYM_SPECIAL_KEYS
#undef X
};

struct tym_special_key_name {
  enum tym_special_key key;
  const char* name;
  size_t name_length;
};

extern const struct tym_special_key_name tym_special_key_list[];
extern const size_t tym_special_key_count;

enum tym_button {
  TYM_I_BUTTON_LEFT_PRESSED,
  TYM_I_BUTTON_MIDDLE_PRESSED,
  TYM_I_BUTTON_RIGHT_PRESSED,
  TYM_I_BUTTON_RELEASED,
};

#define TYM_POSITION_TYPE__CHARFIELD(F) F(CHARFIELD, INTEGER, long  , integer)
#define TYM_POSITION_TYPE__RATIO(F)     F(RATIO    , REAL   , double, real   )

#define TYM_POSITION_TYPE_LIST \
  X(CHARFIELD) \
  X(RATIO)

enum tym_position_type {
  TYM_P_UNSET,
#define X(ID) TYM_P_ ## ID,
  TYM_POSITION_TYPE_LIST
#undef X
  TYM_P_COUNT
};

enum tym_unit_type {
  TYM_U_UNSET,
#define E(PT, U, T, N) TYM_U_ ## U,
#define X(ID) TYM_CONCAT(TYM_POSITION_TYPE__, ID)(E)
  TYM_POSITION_TYPE_LIST
#undef X
#undef E
  TYM_U_COUNT
};

enum tym_axis {
  TYM_AXIS_HORIZONTAL,
  TYM_AXIS_VERTICAL,
  TYM_AXIS_COUNT
};

enum tym_direction {
  TYM_LEFT,
  TYM_RIGHT,
  TYM_TOP,
  TYM_BOTTOM,
  TYM_DIRECTION_COUNT
};

enum tym_rectangle_edge {
  TYM_RECT_TOP_LEFT,
  TYM_RECT_BOTTOM_RIGHT,
  TYM_EDGE_COUNT
};

enum tym_flag {
  TYM_PF_FOCUS,
  TYM_PF_DISALLOW_FOCUS
};

#define TYM_PANE_FOCUS 1

struct tym_unit {
  enum tym_unit_type type;
  union {
#define E(PT, U, T, N) T N;
#define X(ID) TYM_CONCAT(TYM_POSITION_TYPE__, ID)(E)
  TYM_POSITION_TYPE_LIST
#undef X
#undef E
  } value;
};

struct tym_position {
  enum tym_position_type type;
  struct tym_unit axis[TYM_AXIS_COUNT];
};

#define TYM_I_POSITION_SPECIALISATION(X) \
  struct tym_ ## X ## _position { \
    struct tym_position type[TYM_P_COUNT]; \
  }; \
  struct tym_ ## X ## _position_rectangle { \
    struct tym_ ## X ## _position edge[TYM_EDGE_COUNT]; \
  };

TYM_I_POSITION_SPECIALISATION(super)
TYM_I_POSITION_SPECIALISATION(absolute)

#undef TYM_I_POSITION_SPECIALISATION

#define TYM_I_PT_UNIT_NAME(PT, U, T, N) N

#define TYM_POS_REF(POSITION, POSITION_TYPE, AXIS) \
  (POSITION).type[TYM_P_ ## POSITION_TYPE].axis[(AXIS)].value.TYM_CONCAT(TYM_POSITION_TYPE__, POSITION_TYPE)(TYM_I_PT_UNIT_NAME)
#define TYM_RECT_POS_REF(RECT, POSITION_TYPE, POSITION) \
  TYM_POS_REF((RECT).edge[(POSITION) % TYM_EDGE_COUNT], POSITION_TYPE, (POSITION) / TYM_EDGE_COUNT)
#define TYM_RECT_SIZE(RECT, POSITION_TYPE, AXIS) \
  (TYM_POS_REF((RECT).edge[TYM_RECT_BOTTOM_RIGHT], POSITION_TYPE, (AXIS)) - TYM_POS_REF((RECT).edge[TYM_RECT_TOP_LEFT], POSITION_TYPE, (AXIS)))

typedef void(*tym_resize_handler_t)(void* ptr, int pane, const struct tym_super_position_rectangle* input, const struct tym_absolute_position_rectangle* computed );

extern const enum tym_unit_type tym_positon_unit_map[];

TYM_EXPORT int tym_init(void);
TYM_EXPORT int tym_shutdown(void);
TYM_EXPORT int tym_pane_create(const struct tym_super_position_rectangle*restrict super_position);
TYM_EXPORT int tym_pane_destroy(int pane);
TYM_EXPORT int tym_pane_resize(int pane, const struct tym_super_position_rectangle*restrict super_position);
TYM_EXPORT int tym_pane_reset(int pane);
TYM_EXPORT int tym_register_resize_handler( int pane, void* ptr, tym_resize_handler_t handler );
TYM_EXPORT int tym_unregister_resize_handler( int pane, void* ptr, tym_resize_handler_t handler );
TYM_EXPORT int tym_pane_set_flag(int pane, enum tym_flag flag, bool status);
TYM_EXPORT int tym_pane_get_flag(int pane, enum tym_flag flag);
TYM_EXPORT int tym_pane_set_env(int pane);
TYM_EXPORT int tym_pane_get_slavefd(int pane);

TYM_EXPORT int tym_pane_send_key(int pane, int_least16_t key);
TYM_EXPORT int tym_pane_send_keys(int pane, size_t count, const int_least16_t keys[count]);
TYM_EXPORT int tym_pane_send_special_key_by_name(int pane, const char* key_name);
TYM_EXPORT int tym_pane_type(int pane, size_t count, const char keys[count]);
TYM_EXPORT int tym_pane_send_mouse_event(int pane, enum tym_button button, const struct tym_super_position*restrict super_position);

#ifdef __cplusplus
}
#endif

#endif
