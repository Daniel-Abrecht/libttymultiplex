// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <string.h>
#include <internal/main.h>
#include <internal/calc.h>

const enum tym_unit_type tym_positon_unit_map[] = {
#define E(PT, U, T, N) [TYM_I_CONCAT(TYM_P_, PT)] = TYM_I_CONCAT(TYM_U_, U),
#define X(ID) TYM_I_CONCAT(TYM_I_POSITION_TYPE__, ID)(E)
  TYM_I_POSITION_TYPE_LIST
#undef X
#undef E
};

void tym_i_calc_init_absolute_position(struct tym_absolute_position* position){
  for(unsigned p=1; p<TYM_P_COUNT; p++){
    position->type[p].type = p;
    position->type[p].axis[TYM_AXIS_HORIZONTAL].type = tym_positon_unit_map[p];
    position->type[p].axis[TYM_AXIS_VERTICAL].type = tym_positon_unit_map[p];
  }
}

void tym_i_calc_absolut_position(
  struct tym_absolute_position*restrict result,
  const struct tym_absolute_position_rectangle* bounds,
  const struct tym_super_position*restrict input,
  bool bounds_relative
){
  struct tym_absolute_position res;
  for(int axis=0; axis<TYM_AXIS_COUNT; axis++){
    long long cord = TYM_POS_REF(*input, CHARFIELD, axis);
    cord += TYM_POS_REF(*input, RATIO, axis) * TYM_RECT_SIZE(*bounds, CHARFIELD, axis);
    if(!bounds_relative)
      cord += TYM_POS_REF(bounds->edge[TYM_RECT_TOP_LEFT], CHARFIELD, axis);
    TYM_POS_REF(res, CHARFIELD, axis) = cord;
  }
  const struct tym_absolute_position_rectangle* b = bounds_relative ? bounds : &tym_i_bounds;
  for(int axis=0; axis<TYM_AXIS_COUNT; axis++){
    int boundary_size = TYM_RECT_SIZE(*b, CHARFIELD, axis);
    if(boundary_size)
      TYM_POS_REF(res, RATIO, axis) = (double)TYM_POS_REF(res, CHARFIELD, axis) / boundary_size;
  }
  *result = res;
}

void tym_i_calc_rectangle_absolut_position( struct tym_absolute_position_rectangle*restrict result, const struct tym_super_position_rectangle*restrict super_position ){
  struct tym_absolute_position_rectangle res;
  for(int edge=0; edge<TYM_EDGE_COUNT; edge++)
    tym_i_calc_absolut_position(&res.edge[edge], &tym_i_bounds, &super_position->edge[edge], true);
  if( TYM_RECT_POS_REF(res, CHARFIELD, TYM_LEFT) >= TYM_RECT_POS_REF(res, CHARFIELD, TYM_RIGHT )
   || TYM_RECT_POS_REF(res, CHARFIELD, TYM_TOP ) >= TYM_RECT_POS_REF(res, CHARFIELD, TYM_BOTTOM)
  ){
    memset(&res, 0, sizeof(res));
    tym_i_calc_init_absolute_position(&res.edge[TYM_RECT_TOP_LEFT]);
    tym_i_calc_init_absolute_position(&res.edge[TYM_RECT_BOTTOM_RIGHT]);
  }
  *result = res;
}

