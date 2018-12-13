#ifndef TYM_INTERNAL_PARSER_H

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

typedef int (*tym_i_csq_sequence_callback)(struct tym_i_pane_internal* pane);

struct tym_i_command_sequence {
  const char* sequence;
  unsigned short length;
  const char* callback_name;
  tym_i_csq_sequence_callback callback;
};

#endif
