// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_LIST_H
#define TYM_INTERNAL_LIST_H

#include <stddef.h>

/** \file */

int tym_i_list_add(size_t s, size_t*restrict pn, void*restrict*restrict ptr, const void*restrict entry);
int tym_i_list_remove(size_t s, size_t*restrict pn, void*restrict*restrict ptr, size_t entry);

#endif
