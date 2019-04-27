// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <internal/backend.h>
#include <internal/pane.h>
#include <internal/main.h>

int tym_i_pane_refresh_default_proc(struct tym_i_pane_internal* pane){
  (void)pane;
  return 0;
}

int tym_i_pane_set_cursor_mode_default_proc(struct tym_i_pane_internal* pane, enum tym_i_cursor_mode cursor_mode){
  (void)pane;
  (void)cursor_mode;
  return 0;
}

int tym_i_pane_change_screen_default_proc(struct tym_i_pane_internal* pane){
  (void)pane;
  return -1;
}

int tym_i_pane_erase_area_default_proc(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position start,
  struct tym_i_cell_position end,
  bool block,
  struct tym_i_character_format format
){
  return tym_i_backend->pane_set_area_to_character(pane, start, end, block, format, 0, " ");
}

int tym_i_pane_set_area_to_character_default_proc(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position start,
  struct tym_i_cell_position end,
  bool block,
  struct tym_i_character_format format,
  size_t length, const char utf8[length+1]
){
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  if(end.x > w)
    end.x = w;
  if(end.y > h)
    end.y = h;
  for(unsigned y=start.y; y<=end.y; y++){
    unsigned e = (block || y == end.y) ? end.x : w;
    for(unsigned x=start.x; x<e; x++){
      if(tym_i_backend->pane_set_character(pane, (struct tym_i_cell_position){.x=x,.y=y}, format, length, utf8, false) == -1)
        tym_i_debug("pane_set_character failed");
    }
    if(!block)
      start.x = 0;
  }
  return 0;
}

int tym_i_pane_scroll_default_proc(struct tym_i_pane_internal* pane, int n){
  if(n == 0)
    return 0;
  long h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  return tym_i_backend->pane_scroll_region(pane, n, 0, h);
}
