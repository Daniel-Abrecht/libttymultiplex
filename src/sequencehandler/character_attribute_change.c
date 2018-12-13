// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>

int tym_i_csq_character_attribute_change(struct tym_i_pane_internal* pane){
  if(!pane->sequence.integer_count){
    pane->sequence.integer_count = 1;
    pane->sequence.integer[0] = 0;
  }
  size_t i;
  for(i=0; i<pane->sequence.integer_count; i++){
    int c = pane->sequence.integer[i];
    if(c == 0){
      pane->attribute = TYM_I_CA_DEFAULT;
      pane->bgcolor.index = 0;
      pane->fgcolor.index = 0;
    }else if(c == 1){
      pane->attribute |= TYM_I_CA_BOLD;
    }else if(c == 4){
      pane->attribute |= TYM_I_CA_UNDERLINE;
    }else if(c == 5){
      pane->attribute |= TYM_I_CA_BLINK;
    }else if(c == 7){
      pane->attribute |= TYM_I_CA_INVERSE;
    }else if(c == 8){
      pane->attribute |= TYM_I_CA_INVISIBLE;
    }else if(c == 22){
      pane->attribute = TYM_I_CA_DEFAULT;
    }else if(c == 24){
      pane->attribute &= ~TYM_I_CA_UNDERLINE;
    }else if(c == 25){
      pane->attribute &= ~TYM_I_CA_BLINK;
    }else if(c == 27){
      pane->attribute &= ~TYM_I_CA_INVERSE;
    }else if(c == 28){
      pane->attribute &= ~TYM_I_CA_INVISIBLE;
    }else if(c == 38){ // set foreground color
      if(pane->sequence.integer_count - i < 3)
        break;
      pane->fgcolor.index = 255;
      pane->fgcolor.red   = pane->sequence.integer[++i];
      pane->fgcolor.green = pane->sequence.integer[++i];
      pane->fgcolor.blue  = pane->sequence.integer[++i];
    }else if(c == 39){ // set default foreground color
      pane->fgcolor.index = 0;
    }else if(c / 10 == 3){ // set foreground color
      pane->fgcolor.index = c - 30 + 1;
    }else if(c / 10 == 9){ // set foreground color
      pane->fgcolor.index = c - 90 + 11;
      if(c > 17){
        errno = EINVAL;
        return -1;
      }
    }else if(c == 48){ // set background color
      if(pane->sequence.integer_count - i < 3)
        break;
      pane->bgcolor.index = 255;
      pane->bgcolor.red   = pane->sequence.integer[++i];
      pane->bgcolor.green = pane->sequence.integer[++i];
      pane->bgcolor.blue  = pane->sequence.integer[++i];
    }else if(c == 49){ // set default background color
      pane->bgcolor.index = 0;
    }else if(c / 10 == 4){ // set background color
      pane->bgcolor.index = c - 40 + 1;
    }else if(c / 10 == 10){ // set background color
      pane->bgcolor.index = c - 100 + 11;
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
