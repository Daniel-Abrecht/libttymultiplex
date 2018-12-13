#ifndef TYM_INTERNAL_CALC_H
#define TYM_INTERNAL_CALC_H

#include <libttymultiplex.h>

void tym_i_calc_init_pos_cords(struct tym_position position[TYM_P_COUNT][2]);
void tym_i_calc_absolut_position( struct tym_absolute_position*restrict result, const struct tym_superposition*restrict superposition );

#endif
