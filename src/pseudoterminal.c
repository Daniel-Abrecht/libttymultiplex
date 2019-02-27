// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <unistd.h>
#include <curses.h>
#include <internal/main.h>
#include <internal/parser.h>
#include <internal/pseudoterminal.h>
#include <libttymultiplex.h>

int tym_i_pts_send(struct tym_i_pane_internal* pane, size_t size, const void*restrict data){
  if(!pane){
    errno = EINVAL;
    return -1;
  }
  ssize_t ret = 0;
  while((ret=write(pane->master, data, size)) == -1 && (errno == EAGAIN || errno == EINTR));
  return ret == -1 ? -1 : 0;
}

void tym_i_pts_send_mouse_event(struct tym_i_pane_internal* pane, mmask_t buttons, struct tym_i_cell_position pos){
  char buf[64] = {0};
  int len = 0;
  bool motion = pane->last_mouse_event_pos.x != pos.x || pane->last_mouse_event_pos.y != pos.y;
  pane->last_mouse_event_pos = pos;
  enum {
    RELEASED,
    PRESSED,
    CLICK,
    DOUBLECLICK,
    TRIPPLECLICK
  } events = RELEASED;
  int button = -1;
  if( buttons & (BUTTON1_PRESSED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED )){
    button = 0;
    if(buttons & BUTTON1_PRESSED){
      events = PRESSED;
    }else if(buttons & BUTTON1_CLICKED){
      events = CLICK;
    }else if(buttons & BUTTON1_DOUBLE_CLICKED){
      events = DOUBLECLICK;
    }else if(buttons & BUTTON1_TRIPLE_CLICKED){
      events = TRIPPLECLICK;
    }
  }else if( buttons & (BUTTON2_PRESSED | BUTTON2_CLICKED | BUTTON2_DOUBLE_CLICKED | BUTTON2_TRIPLE_CLICKED )){
    button = 1;
    if(buttons & BUTTON2_PRESSED){
      events = PRESSED;
    }else if(buttons & BUTTON2_CLICKED){
      events = CLICK;
    }else if(buttons & BUTTON2_DOUBLE_CLICKED){
      events = DOUBLECLICK;
    }else if(buttons & BUTTON2_TRIPLE_CLICKED){
      events = TRIPPLECLICK;
    }
  }else if( buttons & (BUTTON3_PRESSED | BUTTON3_CLICKED | BUTTON3_DOUBLE_CLICKED | BUTTON3_TRIPLE_CLICKED )){
    button = 2;
    if(buttons & BUTTON3_PRESSED){
      events = PRESSED;
    }else if(buttons & BUTTON3_CLICKED){
      events = CLICK;
    }else if(buttons & BUTTON3_DOUBLE_CLICKED){
      events = DOUBLECLICK;
    }else if(buttons & BUTTON3_TRIPLE_CLICKED){
      events = TRIPPLECLICK;
    }
  }else if( buttons & (BUTTON4_PRESSED | BUTTON4_CLICKED | BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED )){
    button = 3;
    if(buttons & BUTTON4_PRESSED){
      events = PRESSED;
    }else if(buttons & BUTTON4_CLICKED){
      events = CLICK;
    }else if(buttons & BUTTON4_DOUBLE_CLICKED){
      events = DOUBLECLICK;
    }else if(buttons & BUTTON4_TRIPLE_CLICKED){
      events = TRIPPLECLICK;
    }
  }
  int cb_btn = button;
  if(button >= 0 && button <= 2){
    cb_btn = button;
  }else{
    cb_btn = 3; // Button released code
    events = RELEASED;
  }
  switch(pane->mouse_mode){
    case MOUSE_MODE_X10:
      if(events == RELEASED)
        break;
      if(events > PRESSED)
        events = PRESSED;
    case MOUSE_MODE_NORMAL:
      if(pane->last_button == cb_btn)
        break;
    case MOUSE_MODE_BUTTON:
      if(pane->last_button == cb_btn && cb_btn == 3)
        break;
    case MOUSE_MODE_ANY: {
      if(pos.x>254 || pos.y>254)
        break; // Can't encode coordinate in a byte each, overflow!!!
      unsigned char cb = 32;
      if(motion)
        cb += 32; // Motion indicator
      do {
        int s;
        s = snprintf(buf+len, sizeof(buf)-len, CSI "M%c%c%c\n", cb+cb_btn, (unsigned char)(32+pos.x+1), (unsigned char)(32+pos.y+1));
        if(s != -1) len += s;
        if(events <= PRESSED || cb_btn == 3)
          break;
        s = snprintf(buf+len, sizeof(buf)-len, CSI "M%c%c%c\n", cb+3, (unsigned char)(32+pos.x+1), (unsigned char)(32+pos.y+1));
        if(s != -1) len += s;
        events -= 1;
      } while(events > PRESSED);
    } break;
    case MOUSE_MODE_OFF: break;
  }
  pane->last_button = cb_btn;
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

