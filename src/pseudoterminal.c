// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <internal/main.h>
#include <internal/parser.h>
#include <internal/pseudoterminal.h>
#include <libttymultiplex.h>

#define S(X) sizeof(X)-1, X

int tym_i_pts_send(struct tym_i_pane_internal* pane, size_t size, const void*restrict data){
  if(!pane){
    errno = EINVAL;
    return -1;
  }
  ssize_t ret = 0;
  while((ret=write(pane->master, data, size)) == -1 && (errno == EAGAIN || errno == EINTR));
  return ret == -1 ? -1 : 0;
}

int tym_i_pts_send_special_key(struct tym_i_pane_internal* pane, enum tym_special_key key){
  if(!pane){
    errno = EINVAL;
    return -1;
  }
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  switch(key){
    case TYM_KEY_UP: case TYM_KEY_DOWN: case TYM_KEY_RIGHT: case TYM_KEY_LEFT: {
      switch(screen->cursor_key_mode){
        case TYM_I_KEYPAD_MODE_NORMAL: switch(key){
            case TYM_KEY_UP   : return tym_i_pts_send(pane, S("\x1B[A"));
            case TYM_KEY_DOWN : return tym_i_pts_send(pane, S("\x1B[B"));
            case TYM_KEY_RIGHT: return tym_i_pts_send(pane, S("\x1B[C"));
            case TYM_KEY_LEFT : return tym_i_pts_send(pane, S("\x1B[D"));
            default: return -1;
        } break;
        case TYM_I_KEYPAD_MODE_APPLICATION: switch(key){
            case TYM_KEY_UP   : return tym_i_pts_send(pane, S("\x1BOA"));
            case TYM_KEY_DOWN : return tym_i_pts_send(pane, S("\x1BOB"));
            case TYM_KEY_RIGHT: return tym_i_pts_send(pane, S("\x1BOC"));
            case TYM_KEY_LEFT : return tym_i_pts_send(pane, S("\x1BOD"));
            default: return -1;
        } break;
      }
    } break;
    case TYM_KEY_ENTER: return tym_i_pts_send(pane, S("\n"));
    case TYM_KEY_BACKSPACE: return tym_i_pts_send(pane, S("\b"));
  }
  return -1;
}

// Replace mmask_t with a something not curses specific
void tym_i_pts_send_mouse_event(struct tym_i_pane_internal* pane, enum tym_i_button button, struct tym_i_cell_position pos){
  char buf[64] = {0};
  int len = 0;
  bool motion = pane->last_mouse_event_pos.x != pos.x || pane->last_mouse_event_pos.y != pos.y;
  if(pane->last_button != button)
    motion = false;
  pane->last_mouse_event_pos = pos;
  switch(pane->mouse_mode){
    case MOUSE_MODE_X10:
      if(button == TYM_I_BUTTON_RELEASED)
        break;
    case MOUSE_MODE_NORMAL:
      if(pane->last_button == button)
        break;
    case MOUSE_MODE_BUTTON:
      if(pane->last_button == button && button == TYM_I_BUTTON_RELEASED)
        break;
    case MOUSE_MODE_ANY: {
      if(pos.x>254 || pos.y>254)
        break; // Can't encode coordinate in a byte each, overflow!!! (TODO: implement stuff like utf8 encoded positions)
      unsigned char cb = 32;
      if(motion)
        cb += 32; // Motion indicator
      len = snprintf(buf, sizeof(buf), CSI "M%c%c%c\n", cb+button, (unsigned char)(32+pos.x+1), (unsigned char)(32+pos.y+1));
    } break;
    case MOUSE_MODE_OFF: break;
  }
  pane->last_button = button;
  if(len){
    tym_i_pts_send(pane, len, buf);
  }
}

void tym_i_pts_send_key(int c){
  if(!tym_i_focus_pane)
    return;
  if(c <= 0xFF){
    while(write(tym_i_focus_pane->master, (char[]){c}, sizeof(char)) == -1 && (errno == EINTR || errno == EAGAIN));
  }else{
    
  }
}

