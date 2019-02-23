#ifndef TYM_INTERNAL_PSEUDOTERMINAL_H
#define TYM_INTERNAL_PSEUDOTERMINAL_H

#include <internal/pane.h>

int tym_i_pts_send(struct tym_i_pane_internal* pane, size_t size, const void*restrict data);
void tym_i_pts_send_mouse_event(struct tym_i_pane_internal* pane, mmask_t buttons, struct tym_i_cell_position pos);
void tym_i_pts_send_key(int c);

#endif
