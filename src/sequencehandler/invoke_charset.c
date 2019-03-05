// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <string.h>
#include <internal/pane.h>
#include <internal/main.h>

static int invoke_charset(struct tym_i_pane_internal* pane, enum charset_selection cs){
  if(pane->sequence.integer_count != 0){
    errno = EINVAL;
    return -1;
  }
  pane->charset_selection = cs;
  return 0;
}

int tym_i_csq_select_defautl_character_set(struct tym_i_pane_internal* pane){
  if(invoke_charset(pane, 0) == -1)
    return -1;
  memset(pane->charset_g, 0, sizeof(*pane->charset_g)*TYM_I_G_CHARSET_COUNT);
  return 0;
}

int tym_i_csq_select_utf8_character_set(struct tym_i_pane_internal* pane){
  return invoke_charset(pane, TYM_I_CHARSET_SELECTION_UTF8);
}

int tym_i_csq_invoke_charset_G2_as_GL_SL2(struct tym_i_pane_internal* pane){
  return invoke_charset(pane, TYM_I_CHARSET_SELECTION_GL_G2 | (pane->charset_selection & TYM_I_CHARSET_SELECTION_GR_MASK));
}

int tym_i_csq_invoke_charset_G3_as_GL_SL3(struct tym_i_pane_internal* pane){
  return invoke_charset(pane, TYM_I_CHARSET_SELECTION_GL_G3 | (pane->charset_selection & TYM_I_CHARSET_SELECTION_GR_MASK));
}

int tym_i_csq_invoke_charset_G3_as_GR_LS3R(struct tym_i_pane_internal* pane){
  return invoke_charset(pane, TYM_I_CHARSET_SELECTION_GR_G3 | (pane->charset_selection & TYM_I_CHARSET_SELECTION_GL_MASK));
}

int tym_i_csq_invoke_charset_G4_as_GR_LS2R(struct tym_i_pane_internal* pane){
  return invoke_charset(pane, TYM_I_CHARSET_SELECTION_GR_G4 | (pane->charset_selection & TYM_I_CHARSET_SELECTION_GL_MASK));
}

int tym_i_csq_invoke_charset_G1_as_GR_LS1R(struct tym_i_pane_internal* pane){
  return invoke_charset(pane, TYM_I_CHARSET_SELECTION_GR_G3 | (pane->charset_selection & TYM_I_CHARSET_SELECTION_GL_MASK));
}
