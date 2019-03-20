// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <string.h>
#include <internal/pane.h>
#include <internal/main.h>

int tym_i_invoke_charset(struct tym_i_pane_internal* pane, enum charset_selection cs){
  if(pane->sequence.integer_count != 0){
    errno = EINVAL;
    return -1;
  }
  pane->character.charset_selection = cs;
  return 0;
}

int tym_i_csq_select_default_character_set(struct tym_i_pane_internal* pane){
  if(tym_i_invoke_charset(pane, 0) == -1)
    return -1;
  memset(pane->character.charset_g, 0, sizeof(*pane->character.charset_g)*TYM_I_G_CHARSET_COUNT);
  pane->character.not_utf8 = true;
  return 0;
}

int tym_i_csq_select_utf8_character_set(struct tym_i_pane_internal* pane){
  pane->character.not_utf8 = false;
  return 0;
}

int tym_i_csq_tym_i_invoke_charset_G1_as_GL_SL2(struct tym_i_pane_internal* pane){
  return tym_i_invoke_charset(pane, TYM_I_CHARSET_SELECTION_GL_G1 | (pane->character.charset_selection & TYM_I_CHARSET_SELECTION_GR_MASK));
}

int tym_i_csq_tym_i_invoke_charset_G2_as_GL_SL3(struct tym_i_pane_internal* pane){
  return tym_i_invoke_charset(pane, TYM_I_CHARSET_SELECTION_GL_G2 | (pane->character.charset_selection & TYM_I_CHARSET_SELECTION_GR_MASK));
}

int tym_i_csq_tym_i_invoke_charset_G2_as_GR_LS3R(struct tym_i_pane_internal* pane){
  return tym_i_invoke_charset(pane, TYM_I_CHARSET_SELECTION_GR_G2 | (pane->character.charset_selection & TYM_I_CHARSET_SELECTION_GL_MASK));
}

int tym_i_csq_tym_i_invoke_charset_G3_as_GR_LS2R(struct tym_i_pane_internal* pane){
  return tym_i_invoke_charset(pane, TYM_I_CHARSET_SELECTION_GR_G3 | (pane->character.charset_selection & TYM_I_CHARSET_SELECTION_GL_MASK));
}

int tym_i_csq_tym_i_invoke_charset_G0_as_GR_LS1R(struct tym_i_pane_internal* pane){
  return tym_i_invoke_charset(pane, TYM_I_CHARSET_SELECTION_GR_G0 | (pane->character.charset_selection & TYM_I_CHARSET_SELECTION_GL_MASK));
}
