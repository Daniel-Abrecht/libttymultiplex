// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_CALC_H
#define TYM_INTERNAL_CALC_H

#include <libttymultiplex.h>

void tym_i_calc_init_absolute_position(struct tym_absolute_position* position);
void tym_i_calc_rectangle_absolut_position( struct tym_absolute_position_rectangle*restrict result, const struct tym_super_position_rectangle*restrict superposition );
void tym_i_calc_absolut_position(
  struct tym_absolute_position*restrict result,
  const struct tym_absolute_position_rectangle* boundaries,
  const struct tym_super_position*restrict input,
  bool bounds_relative
);

#endif
