// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TERMINFO_HELPER_H
#define TERMINFO_HELPER_H

#include <sys/types.h>
#include <sys/stat.h>

int tym_i_open_terminfo(const char* name, mode_t mode);

#endif
