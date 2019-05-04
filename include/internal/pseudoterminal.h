// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef TYM_INTERNAL_PSEUDOTERMINAL_H
#define TYM_INTERNAL_PSEUDOTERMINAL_H

#include <internal/pane.h>

int tym_i_pts_send(struct tym_i_pane_internal* pane, size_t size, const void*restrict data);
int tym_i_pts_send_key(struct tym_i_pane_internal* pane, int_least16_t key);
int tym_i_pts_send_keys(struct tym_i_pane_internal* pane, size_t count, const int_least16_t keys[count]);
int tym_i_pts_type(struct tym_i_pane_internal* pane, size_t count, const char keys[count]);
int tym_i_pts_send_mouse_event(struct tym_i_pane_internal* pane, enum tym_button button, struct tym_i_cell_position pos);

#endif
