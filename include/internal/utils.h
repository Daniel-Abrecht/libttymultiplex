// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_UTILS_H
#define TYM_INTERNAL_UTILS_H

#include <stddef.h>
#include <libttymultiplex.h>

/** \file */

/**
 * Allocate space for and copy some data
 * 
 * \param s The size of the data to be copyed in bytes
 * \param init The data to be copyed
 * \retruns A new copy of the data
 */
void* tym_i_copy(size_t s, void* init);

/**
 * For a given identifier, this will generate a new identifier which will be
 * different for different lines, but the same for the same line number.
 * This is useful for generating unique static variable names in other macros.
 */
#define TYM_I_LUNIQUE(X) TYM_I_CONCAT(X,__LINE__)

#endif
