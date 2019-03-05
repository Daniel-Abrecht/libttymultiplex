// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <internal/pane.h>
#include <internal/main.h>

static int designate_character_set(struct tym_i_pane_internal* pane, int i){
  if(i > TYM_I_G_CHARSET_COUNT){
    errno = EINVAL;
    return -1;
  }
  if(pane->sequence.integer_count != 1){
    errno = ENOENT;
    return -1;
  }
  int cset = -1;
  switch(*pane->sequence.integer){
    case '0': cset = TYM_I_CHARSET_DEC_SPECIAL_CHARACTER_AND_LINE_DRAWING_SET; break;
    case 'A': cset = TYM_I_CHARSET_UK; break;
    case 'B': cset = TYM_I_CHARSET_USASCII; break;
    case '4': cset = TYM_I_CHARSET_DUTCH; break;
    case '5':
    case 'C': cset = TYM_I_CHARSET_FINNISH; break;
    case 'R': cset = TYM_I_CHARSET_FRENCH; break;
    case 'Q': cset = TYM_I_CHARSET_FRENCH_CANADIAN; break;
    case 'K': cset = TYM_I_CHARSET_GERMAN; break;
    case 'Y': cset = TYM_I_CHARSET_ITALIAN; break;
    case '6':
    case 'E': cset = TYM_I_CHARSET_NORWEGIAN_DANISH; break;
    case 'Z': cset = TYM_I_CHARSET_SPANISH; break;
    case '7':
    case 'H': cset = TYM_I_CHARSET_SWEDISH; break;
    case '=': cset = TYM_I_CHARSET_SWISS; break;
  }
  if(cset == -1)
    return 0;
  tym_i_debug("designate_character_set %d %c %d->%d\n", i, *pane->sequence.integer, pane->charset_g[i], cset);
  pane->charset_g[i] = cset;
  return 0;
}

int tym_i_csq_designate_g0_character_set(struct tym_i_pane_internal* pane){
  return designate_character_set(pane, 0);
}

int tym_i_csq_designate_g1_character_set(struct tym_i_pane_internal* pane){
  return designate_character_set(pane, 1);
}

int tym_i_csq_designate_g2_character_set(struct tym_i_pane_internal* pane){
  return designate_character_set(pane, 2);
}

int tym_i_csq_designate_g3_character_set(struct tym_i_pane_internal* pane){
  return designate_character_set(pane, 3);
}
