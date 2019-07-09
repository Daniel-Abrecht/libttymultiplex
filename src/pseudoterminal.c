// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <internal/main.h>
#include <internal/parser.h>
#include <internal/pseudoterminal.h>
#include <libttymultiplex.h>

/** \file */

#define S(X) sizeof(X)-1, X

/**
 * Write some data to the pseudo terminal master (ptm), so it can be read by the pseudo terminal slave (pts).
 * The pseudo terminal (pty) may modifythe these data though, see termios(3). Use one of the other
 * tym_i_pts_* functions to deal with that.
 */
int tym_i_pts_send(struct tym_i_pane_internal* pane, size_t size, const void*restrict data){
  if(!pane){
    errno = EINVAL;
    return -1;
  }
  ssize_t ret = 0;
  while((ret=write(pane->master, data, size)) == -1 && (errno == EAGAIN || errno == EINTR));
  return ret == -1 ? -1 : 0;
}

/**
 * \see tym_pane_send_key
 */
int tym_i_pts_send_key(struct tym_i_pane_internal* pane, int_least16_t key){
  if(!pane){
    errno = EINVAL;
    return -1;
  }
  if( key & TYM_KEY_MODIFIER_CTRL && ( (key & ~TYM_KEY_MODIFIER_CTRL) < 0x40 || (key & ~TYM_KEY_MODIFIER_CTRL) >= 0x80 ) )
    key &= ~TYM_KEY_MODIFIER_CTRL; // Ignore control key for these keys
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  #define CS(X) case TYM_KEY_ ## X:
  switch((enum tym_special_key)key){
    CS(UP) CS(DOWN) CS(RIGHT) CS(LEFT) CS(HOME) CS(END) {
      switch(screen->cursor_key_mode){
        case TYM_I_KEYPAD_MODE_NORMAL: switch(key){
            case TYM_KEY_UP   : return tym_i_pts_send(pane, S(CSI "A"));
            case TYM_KEY_DOWN : return tym_i_pts_send(pane, S(CSI "B"));
            case TYM_KEY_RIGHT: return tym_i_pts_send(pane, S(CSI "C"));
            case TYM_KEY_LEFT : return tym_i_pts_send(pane, S(CSI "D"));
            case TYM_KEY_HOME : return tym_i_pts_send(pane, S(CSI "H"));
            case TYM_KEY_END  : return tym_i_pts_send(pane, S(CSI "F"));
            default: return -1;
        } break;
        case TYM_I_KEYPAD_MODE_APPLICATION: switch(key){
            case TYM_KEY_UP   : return tym_i_pts_send(pane, S(SS3 "A"));
            case TYM_KEY_DOWN : return tym_i_pts_send(pane, S(SS3 "B"));
            case TYM_KEY_RIGHT: return tym_i_pts_send(pane, S(SS3 "C"));
            case TYM_KEY_LEFT : return tym_i_pts_send(pane, S(SS3 "D"));
            case TYM_KEY_HOME : return tym_i_pts_send(pane, S(SS3 "H"));
            case TYM_KEY_END  : return tym_i_pts_send(pane, S(SS3 "F"));
            default: return -1;
        } break;
      }
    } break;
    case TYM_KEY_PAGE_UP: return tym_i_pts_send(pane, S(CSI "5~"));
    case TYM_KEY_PAGE_DOWN: return tym_i_pts_send(pane, S(CSI "6~"));
    case TYM_KEY_ENTER: return tym_i_pts_send(pane, S("\r"));
    case TYM_KEY_TAB: break;
    case TYM_KEY_BACKSPACE: break;
    case TYM_KEY_ESCAPE: break;
    case TYM_KEY_DELETE: return tym_i_pts_send(pane, S(CSI "3~"));
  }
  if( key & TYM_KEY_MODIFIER_CTRL )
    key = (key & ~TYM_KEY_MODIFIER_CTRL) - 64;
  if(key >= 0 && key < 0x100)
    return tym_i_pts_send(pane, 1, (char[]){key});
#undef CS
  return -1;
}

/**
 * \see tym_pane_send_keys
 */
int tym_i_pts_send_keys(struct tym_i_pane_internal* pane, size_t count, const int_least16_t keys[count]){
  for(size_t i=0; i<count; i++)
    tym_i_pts_send_key(pane, keys[i]);
  return 0;
}

/**
 * \see tym_pane_type
 */
int tym_i_pts_type(struct tym_i_pane_internal* pane, size_t count, const char keys[count]){
  for(size_t i=0; i<count; i++)
    tym_i_pts_send_key(pane, keys[i]);
  return 0;
}

/**
 * Send the escape sequence for a mouse event.
 * 
 * \see tym_pane_send_mouse_event
 * 
 * \todo Currently, only positions from 0 to 254 characters can be specified.
 *       There are other encodings to circumvent this limitation, but those have yet to be implemented.
 */
int tym_i_pts_send_mouse_event(struct tym_i_pane_internal* pane, enum tym_button button, struct tym_i_cell_position pos){
  char buf[64] = {0};
  int len = 0;
  bool motion = pane->last_mouse_event_pos.x != pos.x || pane->last_mouse_event_pos.y != pos.y;
  if(pane->last_button != button)
    motion = false;
  pane->last_mouse_event_pos = pos;
  switch(pane->mouse_mode){
    case TYM_I_MOUSE_MODE_X10:
      if(button == TYM_BUTTON_RELEASED)
        break;
    case TYM_I_MOUSE_MODE_NORMAL:
      if(pane->last_button == button)
        break;
    case TYM_I_MOUSE_MODE_BUTTON:
      if(pane->last_button == button && button == TYM_BUTTON_RELEASED)
        break;
    case TYM_I_MOUSE_MODE_ANY: {
      if(pos.x>254 || pos.y>254){ // Can't encode coordinate in a byte each, overflow!!!
        errno = EINVAL;
        return -1;
      }
      unsigned char cb = 32;
      if(motion)
        cb += 32; // Motion indicator
      len = snprintf(buf, sizeof(buf), CSI "M%c%c%c", cb+button, (unsigned char)(32+pos.x+1), (unsigned char)(32+pos.y+1));
    } break;
    case TYM_I_MOUSE_MODE_OFF: break;
  }
  pane->last_button = button;
  if(len)
    return tym_i_pts_send(pane, len, buf);
  return 0;
}
