// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef LIBTTYMULTIPLEX_H
#define LIBTTYMULTIPLEX_H

/** \file */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#ifndef TYM_EXPORT
#ifdef TYM_BUILD
#define TYM_EXPORT __attribute__((visibility ("default")))
#else
#define TYM_EXPORT
#endif
#endif

#define TYM_I_CONCAT_SUB(A,B) A ## B
#define TYM_I_CONCAT(A,B) TYM_I_CONCAT_SUB(A,B)
#define TYM_I_UNPACK(...) __VA_ARGS__
#define TYM_I_DOKUMENTATION(...) /** __VA_ARGS__ */

#ifdef __cplusplus
extern "C" {
#endif

#define TYM_I_SPECIAL_KEYS(X) \
  X(ENTER    , '\n'  ) \
  X(HOME     , '\r'  ) \
  X(BACKSPACE, '\b'  ) \
  X(TAB      , '\t'  ) \
  X(ESCAPE   , '\x1B') \
  X(DELETE   , '\x7F') \
  X(UP       , 0x100 ) \
  X(DOWN     , 0x101 ) \
  X(RIGHT    , 0x102 ) \
  X(LEFT     , 0x103 ) \
  X(END      , 0x104 ) \
  X(PAGE_UP  , 0x105 ) \
  X(PAGE_DOWN, 0x106 )

#define TYM_KEY_MODIFIER_CTRL 0x8000

/**
 * These are constants representing special keys.
 * These constants are not indeces for tym_special_key_list,
 * but can be used with tym_pane_send_key and tym_pane_send_keys.
 */
enum tym_special_key {
#define X1(ID, VAL) TYM_KEY_ ## ID = VAL,
TYM_I_SPECIAL_KEYS(X1)
#undef X1
};

/** Informations about supported special keys. */
struct tym_special_key_name {
  /** The key constant */
  enum tym_special_key key;
  /** The key name, null terminated */
  const char* name;
  /** The length of the key name */
  size_t name_length;
};

/** A list of all special keys. */
extern const struct tym_special_key_name tym_special_key_list[];
/** The length of the list of special keys. */
extern const size_t tym_special_key_count;

/** All possible mouse button state */
enum tym_button {
  TYM_BUTTON_LEFT_PRESSED,
  TYM_BUTTON_MIDDLE_PRESSED,
  TYM_BUTTON_RIGHT_PRESSED,
  TYM_BUTTON_RELEASED,
};

#define TYM_I_LOG_LEVEL \
  X(DEBUG) \
  X(INFO) \
  X(WARN) \
  X(ERROR) \
  X(FATAL)
enum tym_log_level {
#define X(Y) TYM_LOG_ ## Y,
  TYM_I_LOG_LEVEL
#undef X
};

#define TYM_I_UNIT_TYPE__INTEGER(F) F(INTEGER, long  , integer)
#define TYM_I_UNIT_TYPE__REAL(F)    F(REAL   , double, real   )

#define TYM_I_UNIT_TYPE_LIST(F) \
  TYM_I_UNIT_TYPE__INTEGER(F) \
  TYM_I_UNIT_TYPE__REAL(F)

#define TYM_I_UNIT_LOOKUP(N,F) TYM_I_CONCAT(TYM_I_UNIT_TYPE__, N)(F)

#define TYM_I_POSITION_TYPE__CHARFIELD(F) F(CHARFIELD, INTEGER, (The position from the top left corner in characters))
#define TYM_I_POSITION_TYPE__RATIO(F)     F(RATIO    , REAL   , (The position as a factor of the length along a tym_axis. 0 -> top left, 1 -> bottom right.))

#define TYM_I_POSITION_TYPE_LIST(F) \
  TYM_I_POSITION_TYPE__CHARFIELD(F) \
  TYM_I_POSITION_TYPE__RATIO(F)

#define TYM_I_POSITION_LOOKUP(N,F) TYM_I_CONCAT(TYM_I_POSITION_TYPE__, N)(F)

#ifndef TYM_LOG_FUNC
/** The default log function, can be overwritten. */
#define TYM_LOG_FUNC tym_u_log
#endif

#ifndef TYM_LOG_PROJECT
/** The default project name to prefix to log messages. Defaults to 0 (unspecified). */
#define TYM_LOG_PROJECT 0
#endif

/** Convinience macro for simpler usage of #TYM_LOG_FUNC. */
#define TYM_U_LOG(level,...) TYM_LOG_FUNC(TYM_LOG_PROJECT, level, __FILE__, __LINE__, __VA_ARGS__)
/** Convinience macro for simpler usage of #TYM_LOG_FUNC. Intended usage similar to perror. */
#define TYM_U_PERROR(level,message) TYM_LOG_FUNC(TYM_LOG_PROJECT, level, __FILE__, __LINE__, "%s: %s\n", message, strerror(errno))

/**
 * Different types of positions for tym_position, which differ in what they specify the position in relation to.
 * There is always only one valid #tym_unit_type for any given #tym_position_type.
 */
enum tym_position_type {
  /** The position type was never set */
  TYM_P_UNSET,
#define X2(PT, U, DOC) TYM_I_DOKUMENTATION DOC TYM_P_ ## PT,
  TYM_I_POSITION_TYPE_LIST(X2)
#undef X2
  /** The number of position types. */
  TYM_P_COUNT
};

/** The unit types for tym_unit. */
enum tym_unit_type {
  /** The unit type was never set */
  TYM_U_UNSET,
#define X3(U, T, N) /** Use tym_unit::value::##N. */ TYM_U_ ## U,
  TYM_I_UNIT_TYPE_LIST(X3)
#undef X3
  /** The number of position types */
  TYM_U_COUNT
};

/** The axis of our space, one for each dimension */
enum tym_axis {
  TYM_AXIS_HORIZONTAL /** 0 */,
  TYM_AXIS_VERTICAL /** 1 */,
  TYM_AXIS_COUNT /** 2 */
};

/** Every direction, two for each #tym_axis */
enum tym_direction {
  TYM_LEFT /** 0 */,
  TYM_RIGHT /** 1 */,
  TYM_TOP /** 2 */,
  TYM_BOTTOM /** 3 */,
  TYM_DIRECTION_COUNT /** 4 */
};

/** The edges of the tym_*_position_rectangle types. */
enum tym_rectangle_edge {
  TYM_RECT_TOP_LEFT /** 1 */,
  TYM_RECT_BOTTOM_RIGHT /** 2 */,
  TYM_RECT_EDGE_COUNT /** 3 */
};

/** Flags for #tym_pane_set_flag */
enum tym_flag {
  TYM_PF_FOCUS, //!< Try to set the focus on a pane
  TYM_PF_DISALLOW_FOCUS, //!< Disalow setting the focus to a pane. Removes the focus from a pane if it already has it.
};

/**
 * TYM_PANE_FOCUS always refers to the pane which is currently in focus.
 **/
#define TYM_PANE_FOCUS 1

/**
 * A value of the type specified by #tym_unit_type.
 */
struct tym_unit {
  /** The type specifies which value has to be used. */
  enum tym_unit_type type;
  /** The value, one field for each type. */
  union {
#define X4(U, T, N)  /** Field for unit #TYM_U_ ## U */ T N;
  TYM_I_UNIT_TYPE_LIST(X4)
#undef X4
  } value;
};

/* A position specified as an #tym_position_type. */
struct tym_position {
  /** The position type of the this position, see #tym_position_type. */
  enum tym_position_type type;
  /** All coordinates, one for each #tym_axis. */
  struct tym_unit axis[TYM_AXIS_COUNT];
};

#define TYM_I_POSITION_SPECIALISATION(X, DOKUMENTATION) \
  TYM_I_DOKUMENTATION DOKUMENTATION \
  struct tym_ ## X ## _position { \
    /** The position specified using every possible type. */ \
    struct tym_position type[TYM_P_COUNT]; \
  }; \
  /** tym_ ## X ## _position_rectangle specifies a rectangular region by specifying it's top left and bottom right corner using tym_ ## X ## _position. <br/><br/>To access this structure, consider using the #TYM_RECT_POS_REF macro. */ \
  struct tym_ ## X ## _position_rectangle { \
    /** The position of the top left (TYM_RECT_TOP_LEFT) and bottom right (TYM_RECT_BOTTOM_RIGHT) corners. */ \
    struct tym_ ## X ## _position edge[TYM_RECT_EDGE_COUNT]; \
  };

TYM_I_POSITION_SPECIALISATION(super, (
  The final position of a tym_super_position is the sum of all tym_position.
  There is one for each position type. This allows to specify a position in a way
  that is agnostic to screen size and other units whose size may differ in different
  situations. This isn not enough for implementing responsive designs, though.
  <br/><br/>
  To access this structure, consider using the #TYM_POS_REF macro.
))

TYM_I_POSITION_SPECIALISATION(absolute, (
  A tym_absolute_position contains the absolute computed position in every possible position type.
  <br/><br/>
  To access this structure, consider using the #TYM_POS_REF macro.
 ))

#undef TYM_I_POSITION_SPECIALISATION

#define TYM_I_UNIT_NAME(U, T, N) N
#define TYM_I_PT_UNIT_NAME(PT, U, DOC) TYM_I_UNIT_LOOKUP(U, TYM_I_UNIT_NAME)

/**
 * For simpler usage of the specialisations of tym_position,
 * currently, these are tym_super_position and tym_absolute_position.
 * This macro automatically selects the correct field for the given position type and axis.
 * It is allowed to reference, set and read the selected field. The type of the field depends on the position type.
 * 
 * \param POSITION A specialisations of tym_position such as tym_super_position or tym_absolute_position.
 * \param POSITION_TYPE One of the fields of #tym_position_type, but without TYM_P_ prefix. Currently, only CHARFIELD and RATIO are possible values.
 * \param AXIS One of the values of #tym_axis.
 **/
#define TYM_POS_REF(POSITION, POSITION_TYPE, AXIS) \
  (POSITION).type[TYM_P_ ## POSITION_TYPE].axis[(AXIS)].value.TYM_I_POSITION_LOOKUP(POSITION_TYPE, TYM_I_PT_UNIT_NAME)

/**
 * For simpler usage of the tym_*_position_rectangle objects,
 * currently, these are tym_super_position_rectangle and tym_absolute_position_rectangle.
 * This macro automatically selects the correct field for the given position type and edge of the rectangle.
 * It is allowed to reference, set and read the selected field. The type of the field depends on the position type.
 * 
 * \param RECT One of the tym_*_position_rectangle objects such as tym_super_position_rectangle or tym_absolute_position_rectangle.
 * \param POSITION_TYPE One of the fields of #tym_position_type, but without TYM_P_ prefix. Currently, only CHARFIELD and RATIO are possible values.
 * \param POSITION One of the values of #tym_direction.
 **/
#define TYM_RECT_POS_REF(RECT, POSITION_TYPE, POSITION) \
  TYM_POS_REF((RECT).edge[(POSITION) % TYM_RECT_EDGE_COUNT], POSITION_TYPE, (POSITION) / TYM_RECT_EDGE_COUNT)

/**
 * The size of a tym_*_position_rectangle object in one of the position types along one of the axis directions.
 * This is only really usful for tym_absolute_position_rectangle opjects.
 * 
 * \param RECT One of the tym_*_position_rectangle objects such as tym_super_position_rectangle or tym_absolute_position_rectangle.
 * \param POSITION_TYPE One of the fields of #tym_position_type, but without TYM_P_ prefix. Currently, only CHARFIELD and RATIO are possible values.
 * \param AXIS One of the values of #tym_axis.
 **/
#define TYM_RECT_SIZE(RECT, POSITION_TYPE, AXIS) \
  (TYM_POS_REF((RECT).edge[TYM_RECT_BOTTOM_RIGHT], POSITION_TYPE, (AXIS)) - TYM_POS_REF((RECT).edge[TYM_RECT_TOP_LEFT], POSITION_TYPE, (AXIS)))

/**
 * Type for callback function for #tym_pane_register_resize_handler and #tym_pane_unregister_resize_handler.
 * This callback is called if a panes computed position and/or size changes or has to be recomputed.
 * 
 * It is allowed to call other tym_* functions from within this callback. Avoid anything
 * that takes a long time though, the main loop can't process any requests before this
 * callback returns. Do not resize any panes because of a pane resize event, you could
 * end up with inconsistencies or infinite recursions.
 * 
 * \param ptr The pointer passed to tym_pane_register_resize_handler.
 * \param pane The pane which was resized or changed position.
 * \param input The position & size of the pane as specified, see tym_super_position and tym_super_position_rectangle for details.
 * \param computed The computed & absolute position & size, see tym_absolute_position and tym_absolute_position_rectangle for details.
 */
typedef void(*tym_pane_resize_handler_t)(void* ptr, int pane, const struct tym_super_position_rectangle* input, const struct tym_absolute_position_rectangle* computed );

/**
 * Type for callback function for #tym_register_resize_handler and #tym_unregister_resize_handler.
 * This callback is called if the terminal/screens computed position and/or size changes or has to be recomputed.
 * 
 * It is allowed to call other tym_* functions from within this callback. Avoid anything
 * that takes a long time though, the main loop can't process any requests before this
 * callback returns. This function is called before the panes are resized and setting
 * new sizes for them in this callback is possible, but it isn't receommended.
 * 
 * \param ptr The pointer passed to tym_register_resize_handler.
 * \param computed The computed & absolute position & size, see tym_absolute_position and tym_absolute_position_rectangle for details.
 */
typedef void(*tym_resize_handler_t)(void* ptr, const struct tym_absolute_position_rectangle* computed );

/** Mapping from #tym_position_type to #tym_unit_type. */
extern const enum tym_unit_type tym_positon_unit_map[];



/**
 * This function has to be called before any other function, except for some
 * utility functions, including the logging functions, which can always be used.
 * These functions are prefixec with tym_u_*.
 * 
 * This function starts the internal main/event loop thread.
 * If another function is called first, it will return -1 and set errno to EINVAL.
 * All functions return -1 on error and set errno accordingly.
 * 
 * This function also initialises the backend. It'll take the first backend which
 * reports successfull initialisation. The usage of a specific backend can be enforced
 * using the TM_BACKEND environment variable.
 * 
 * This function may also return -1 and set errno to EAGAIN if #tym_shutdown
 * or #tym_freeze is currently in progress.
 */
TYM_EXPORT int tym_init(void);

/**
 * This function should be called after libttymultiplex is no longer needed.
 * It is also automatically called before the program exits normally, but reying
 * on that isn't recommended.
 * 
 * This function may also return -1 and set errno to EAGAIN if #tym_shutdown
 * or #tym_freeze is currently in progress.
 */
TYM_EXPORT int tym_shutdown(void);

/**
 * Exit the main loop, but keep all states.
 * After this operation, it is safe to call fork.
 * In this state, only #tym_init, #tym_shutdown, #tym_pane_set_env and 
 * #tym_pane_get_slavefd are should be used, almost everything else will
 * return -1 and set errno to EINVAL, unless the documentation of the function
 * say otherwise.
 * <br/><br/>
 * Don't wait too long to call #tym_init or #tym_shutdown after this function,
 * when the pseudo terminal masters aren't read, anything trying to write
 * to the pseudo terminal slaves may end up waiting.
 * <br/><br/>
 * Don't call #tym_shutdown if another program still has a running instance of this
 * library. Use tym_zap in that case.
 *
 * Make sure no other threads are trying to access any functions of this library
 * while you attemp a fork, it would leave the internal mutex lock in an undefined
 * state. In theory, the internal mutex lock should be reset to prevent any
 * undefined behaviour, which would introduce even more restrictions for the
 * usage of this function, but this is currently not done because it's unlikely
 * to cause any actual problems.

 * This function may also return -1 and set errno to EAGAIN if #tym_shutdown
 * or #tym_freeze is currently in progress.
 */
TYM_EXPORT int tym_freeze(void);

/**
 * Free & reset everything, but don't perform any deinitialisation.
 * 
 * Can only be called when libttymultiplex is in frozen mode after tym_freeze was called.
 **/
TYM_EXPORT int tym_zap(void);

/**
 * Create a new pane. A pane is a region on the screen which contains a virtual
 * terminal. libttymultiplex is an xterm-compatible terminal emulator. Some escape
 * sequences aren't implemented yet though.
 * 
 * \param super_position The position & size of the pane. See tym_super_position
 *   and tym_super_position_rectangle for how this coordinate system works.
 * \returns The pane handle, or -1 on error
 */
TYM_EXPORT int tym_pane_create(const struct tym_super_position_rectangle*restrict super_position);

/** Destroy a pane */
TYM_EXPORT int tym_pane_destroy(int pane);

/**
 * Changes the pane size & position.
 * 
 * \param super_position The position & size of the pane. See tym_super_position
 *   and tym_super_position_rectangle for how this coordinate system works. 
 */
TYM_EXPORT int tym_pane_resize(int pane, const struct tym_super_position_rectangle*restrict super_position);

/**
 * Clears the pane content and resets almost any state it had, such as cursor position and so on.
 * It does not reset any flags previously set.
 */
TYM_EXPORT int tym_pane_reset(int pane);

/**
 * Registers a callback function that is called from the libttymultiplex main/event loop
 * whenever the size and position of the pane has to be recalculated. It is also
 * called directly after it's first registration.
 * 
 * \see tym_pane_resize_handler_t
 * 
 * \param ptr A pointer which will be passed to the callback function
 * \param handler The callback function
 */
TYM_EXPORT int tym_pane_register_resize_handler( int pane, void* ptr, tym_pane_resize_handler_t handler );

/**
 * Removes every registered matching callback function pointer pair. If ptr is 0,
 * any matching callback function is removed regardless of the ptr value used at registration time.
 * 
 * \param ptr The same pointer that was passed during registration or 0
 * \param handler The callback function which shall be unregistred
 */
TYM_EXPORT int tym_pane_unregister_resize_handler( int pane, void* ptr, tym_pane_resize_handler_t handler );

/**
 * Registers a callback function that is called from the libttymultiplex main/event loop
 * whenever the size and position of the terminal/screen changes. It is also
 * called directly after it's first registration.
 * 
 * \see tym_resize_handler_t
 * 
 * \param ptr A pointer which will be passed to the callback function
 * \param handler The callback function
 */
TYM_EXPORT int tym_register_resize_handler( void* ptr, tym_resize_handler_t handler );

/**
 * Removes every registered matching callback function pointer pair. If ptr is 0,
 * any matching callback function is removed regardless of the ptr value used at registration time.
 * 
 * \param ptr The same pointer that was passed during registration or 0
 * \param handler The callback function which shall be unregistred
 */
TYM_EXPORT int tym_unregister_resize_handler( void* ptr, tym_resize_handler_t handler );

/**
 * Set a flag on a pane. 
 */
TYM_EXPORT int tym_pane_set_flag(int pane, enum tym_flag flag, bool status);

/**
 * Get the current state of a flag.
 * 
 * \returns The flag state (0 or 1) or -1 in case of a error.
 */
TYM_EXPORT int tym_pane_get_flag(int pane, enum tym_flag flag);

/**
 * Set up the environment and file descriptors so any input and output will go to
 * the selected pane. This function also sets the controling terminal.
 * This is intended to be used after a fork. If you do a fork, make sure to
 * freeze libttymultiplex first using the tym_freeze function.
 */
TYM_EXPORT int tym_pane_set_env(int pane);

/**
 * The pts (pseudo terminal (pty) slave) file descriptor.
 * Anything written to this file descriptor is displayed on the pane, any input
 * sent to the pane can be read from this file descriptor, and so on.
 * 
 * \returns The pts fd or -1 on error
 **/
TYM_EXPORT int tym_pane_get_slavefd(int pane);

/**
 * This function can be used to optain the environment variables tym_pane_set_env would set.
 */
TYM_EXPORT int tym_pane_get_default_env_vars(int pane, void* ptr, int(*callback)(int pane, void* ptr, size_t count, const char* env[count][2]));

/**
 * Send a key press to the pane. You can also use the #tym_special_key constants.
 * Control characters are possible, but not recommended.
 */
TYM_EXPORT int tym_pane_send_key(int pane, uint_least16_t key);

/**
 * Send multiple keys. Control characters are possible, but not recommended.
 * 
 * \see tym_pane_send_key
 * 
 * \param count The umber of keys to be sent
 * \param keys The keys to be sent
 */
TYM_EXPORT int tym_pane_send_keys(int pane, size_t count, const uint_least16_t keys[count]);

/**
 * Send a special key to the pane by key name. See #tym_special_key for possible key names.
 * Omit the prefix TYM_KEY_. 
 */
TYM_EXPORT int tym_pane_send_special_key_by_name(int pane, const char* key_name);

/**
 * Send multiple characters to the pane. utf-8 is allowed. Control characters are possible, but not recommended.
 */
TYM_EXPORT int tym_pane_type(int pane, size_t count, const char keys[count]);

/**
 * Send a mouse button event at the specified position.
 */
TYM_EXPORT int tym_pane_send_mouse_event(int pane, enum tym_button button, const struct tym_super_position*restrict super_position);

/**
 * Similar to tym_u_va_log, but doesn't add any extra formatting.
 */
TYM_EXPORT void tym_u_va_rawlog(enum tym_log_level level, const char* format, va_list args);

#ifdef __GNUC__
TYM_EXPORT void tym_u_rawlog(enum tym_log_level level, const char* format, ...) __attribute__((format(printf, 2, 3)));
#else
/**
 * \see tym_u_va_rawlog
 */
TYM_EXPORT void tym_u_rawlog(enum tym_log_level level, const char* format, ...);
#endif

/**
 * For logging purposes. Currently just logs to whatever filedescriptor is specified
 * using the TM_DEBUGFD environment variable. This may get extended to log to syslog,
 * filtering and so on.
 * 
 * This log function exists because stderr may already be used for other things and syslog isn't
 * particularely well suited for large quantities of output & debugging.
 */
TYM_EXPORT void tym_u_va_log(const char* project, enum tym_log_level level, const char* file, unsigned line, const char* format, va_list args);


#ifdef __GNUC__
TYM_EXPORT void tym_u_log(const char* project, enum tym_log_level level, const char* file, unsigned line, const char* format, ...) __attribute__((format(printf, 5, 6)));
#else
/**
 * There are the convinience macros TYM_U_LOG and TYM_U_PERROR for simpler usage.
 * 
 * \see tym_va_log
 */
TYM_EXPORT void tym_u_log(const char* project, enum tym_log_level level, const char* file, unsigned line, const char* format, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif
