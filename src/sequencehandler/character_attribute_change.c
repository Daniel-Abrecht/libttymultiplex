// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_character_attribute_change(struct tym_i_pane_internal* pane){
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  if(!pane->sequence.integer_count){
    pane->sequence.integer_count = 1;
    pane->sequence.integer[0] = 0;
  }
  size_t i;
  for(i=0; i<pane->sequence.integer_count; i++){
    int c = pane->sequence.integer[i];
    if(c == 0){
      screen->character_format.attribute = TYM_I_CA_DEFAULT;
      screen->character_format.bgcolor.index = 0;
      screen->character_format.fgcolor.index = 0;
    }else if(c == 1){
      screen->character_format.attribute |= TYM_I_CA_BOLD;
    }else if(c == 3){
      screen->character_format.attribute |= TYM_I_CA_ITALIC;
    }else if(c == 4){
      screen->character_format.attribute |= TYM_I_CA_UNDERLINE;
    }else if(c == 5){
      screen->character_format.attribute |= TYM_I_CA_BLINK;
    }else if(c == 7){
      screen->character_format.attribute |= TYM_I_CA_INVERSE;
    }else if(c == 8){
      screen->character_format.attribute |= TYM_I_CA_INVISIBLE;
    }else if(c == 21){
      screen->character_format.attribute &= ~TYM_I_CA_BOLD;
    }else if(c == 22){
      screen->character_format.attribute = TYM_I_CA_DEFAULT;
    }else if(c == 23){
      screen->character_format.attribute &= ~TYM_I_CA_ITALIC;
    }else if(c == 24){
      screen->character_format.attribute &= ~TYM_I_CA_UNDERLINE;
    }else if(c == 25){
      screen->character_format.attribute &= ~TYM_I_CA_BLINK;
    }else if(c == 27){
      screen->character_format.attribute &= ~TYM_I_CA_INVERSE;
    }else if(c == 28){
      screen->character_format.attribute &= ~TYM_I_CA_INVISIBLE;
    }else if(c == 38){ // set foreground color
      if(pane->sequence.integer_count - i < 3)
        break;
      screen->character_format.fgcolor.index = 255;
      screen->character_format.fgcolor.red   = pane->sequence.integer[++i];
      screen->character_format.fgcolor.green = pane->sequence.integer[++i];
      screen->character_format.fgcolor.blue  = pane->sequence.integer[++i];
    }else if(c == 39){ // set default foreground color
      screen->character_format.fgcolor.index = 0;
    }else if(c / 10 == 3){ // set foreground color
      screen->character_format.fgcolor.index = c - 30 + 1;
    }else if(c / 10 == 9){ // set foreground color
      screen->character_format.fgcolor.index = c - 90 + 11;
      if(c > 17){
        errno = EINVAL;
        return -1;
      }
    }else if(c == 48){ // set background color
      if(pane->sequence.integer_count - i < 3)
        break;
      screen->character_format.bgcolor.index = 255;
      screen->character_format.bgcolor.red   = pane->sequence.integer[++i];
      screen->character_format.bgcolor.green = pane->sequence.integer[++i];
      screen->character_format.bgcolor.blue  = pane->sequence.integer[++i];
    }else if(c == 49){ // set default background color
      screen->character_format.bgcolor.index = 0;
    }else if(c / 10 == 4){ // set background color
      screen->character_format.bgcolor.index = c - 40 + 1;
    }else if(c / 10 == 10){ // set background color
      screen->character_format.bgcolor.index = c - 100 + 11;
      if(c > 17){
        errno = EINVAL;
        return -1;
      }
    }else break;
  }
  if(i == pane->sequence.integer_count)
    return 0;
  if(i){
    errno = EINVAL;
    return -1;
  }
  errno = ENOENT;
  return -1;
}
