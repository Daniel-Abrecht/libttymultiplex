// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <string.h>
#include <internal/main.h>
#include <internal/calc.h>

void tym_i_calc_init_pos_cords(struct tym_position position[TYM_P_COUNT][2]){
  for(unsigned p=1; p<TYM_P_COUNT; p++){
    position[p][0].type = p;
    position[p][1].type = p;
    position[p][0].axis[0].type = tym_positon_unit_map[p];
    position[p][1].axis[0].type = tym_positon_unit_map[p];
    position[p][0].axis[1].type = tym_positon_unit_map[p];
    position[p][1].axis[1].type = tym_positon_unit_map[p];
  }
}

void tym_i_calc_absolut_position( struct tym_absolute_position*restrict result, const struct tym_superposition*restrict superposition ){
  memset(result, 0, sizeof(*result));
  tym_i_calc_init_pos_cords(result->position);
  for(int i=0; i<2; i++){
    result->position[TYM_P_CHARFIELD][i].axis[0].value.integer = superposition->position[TYM_P_CHARFIELD][i].axis[0].value.integer + superposition->position[TYM_P_RATIO][i].axis[0].value.real * tym_i_ttysize.ws_col;
    if(result->position[TYM_P_CHARFIELD][i].axis[0].value.integer < 0)
      result->position[TYM_P_CHARFIELD][i].axis[0].value.integer = 0;
    if(result->position[TYM_P_CHARFIELD][i].axis[0].value.integer > tym_i_ttysize.ws_col)
      result->position[TYM_P_CHARFIELD][i].axis[0].value.integer = tym_i_ttysize.ws_col;
    result->position[TYM_P_CHARFIELD][i].axis[1].value.integer = superposition->position[TYM_P_CHARFIELD][i].axis[1].value.integer + superposition->position[TYM_P_RATIO][i].axis[1].value.real * tym_i_ttysize.ws_row;
    if(result->position[TYM_P_CHARFIELD][i].axis[1].value.integer < 0)
      result->position[TYM_P_CHARFIELD][i].axis[1].value.integer = 0;
    if(result->position[TYM_P_CHARFIELD][i].axis[1].value.integer > tym_i_ttysize.ws_row)
      result->position[TYM_P_CHARFIELD][i].axis[1].value.integer = tym_i_ttysize.ws_row;
  }
  if( result->position[TYM_P_CHARFIELD][0].axis[0].value.integer >= result->position[TYM_P_CHARFIELD][1].axis[0].value.integer
   || result->position[TYM_P_CHARFIELD][0].axis[1].value.integer >= result->position[TYM_P_CHARFIELD][1].axis[1].value.integer
  ){
    for(int i=0; i<2; i++)
    for(int j=0; j<2; j++)
      result->position[TYM_P_CHARFIELD][i].axis[j].value.integer = 0;
  }
  for(int i=0; i<2; i++){
    result->position[TYM_P_RATIO][i].axis[0].value.real = (double)result->position[TYM_P_CHARFIELD][i].axis[0].value.integer / tym_i_ttysize.ws_col;
    result->position[TYM_P_RATIO][i].axis[1].value.real = (double)result->position[TYM_P_CHARFIELD][i].axis[0].value.integer / tym_i_ttysize.ws_row;
  }
}

