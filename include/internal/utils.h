// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_UTILS_H
#define TYM_INTERNAL_UTILS_H

#include <stddef.h>

void* tym_i_copy(size_t s, void* init);

#define TYM_I_LUNIQUE(X) TYM_I_CONCAT(X,__LINE__)
#define TYM_I_UNPACK(...) __VA_ARGS__

#endif
