// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <unistd.h>
#include <curses.h>
#include <internal/main.h>
#include <internal/pseudoterminal.h>
#include <libttymultiplex.h>

int tym_i_pts_send(struct tym_i_pane_internal* pane, size_t size, const void*restrict data){
  if(!pane){
    errno = EINVAL;
    return -1;
  }
  ssize_t ret = 0;
  while((ret=write(pane->master, data, size)) == -1 && (errno == EAGAIN || errno == EINTR));
  return ret == -1 ? -1 : 0;
}

void tym_i_pts_send_mouse_event(struct tym_i_pane_internal* pane, mmask_t buttons, unsigned x, unsigned y){
  (void)buttons;
  (void)pane;
  (void)x;
  (void)y;
//  printf("%d %u %u\r", pane->id, x, y);
}

void tym_i_pts_send_key(int c){
  if(!tym_i_focus_pane)
    return;
  if(c <= 0xFF){
    while(write(tym_i_focus_pane->master, (char[]){c}, sizeof(char)) == -1 && (errno == EINTR || errno == EAGAIN));
  }else{
    
  }
}

