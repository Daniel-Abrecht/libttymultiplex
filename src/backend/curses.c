// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <ncurses.h>
#include <stdlib.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/backend.h>

#define COLORPAIR_MAPPING_NEGATIVE_FLAG 0x80

static int colortable8[9] = {
  -1, // default color
  COLOR_BLACK,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW,
  COLOR_BLUE,
  COLOR_MAGENTA,
  COLOR_CYAN,
  COLOR_WHITE
};
static uint8_t colorpair8x8_triangular_number_mirror_mapping_index[9][9];
static mmask_t tym_i_mouseeventmask;

struct curses_backend_pane {
  WINDOW* window;
};


static int init(void){
  if(!initscr())
    goto error;
  cbreak();
  nodelay(stdscr, true);
  noecho();
  keypad(stdscr, true);
  if(has_colors()){
    start_color();
    use_default_colors();
    if(COLOR_PAIRS >= 45){
      for(uint8_t i=0, j=0, k=1; k<=45; k++){
        colorpair8x8_triangular_number_mirror_mapping_index[i][j] = k | COLORPAIR_MAPPING_NEGATIVE_FLAG;
        colorpair8x8_triangular_number_mirror_mapping_index[j][i] = k;
        init_pair(k,colortable8[j],colortable8[i]);
        if(i == j){
          j += 1;
          i  = 0;
        }else{
          i += 1;
        }
      }
    }else if(COLOR_PAIRS >= 16){
      // TODO
    }
  }
  leaveok(stdscr, false);
  refresh();
  mousemask(ALL_MOUSE_EVENTS|REPORT_MOUSE_POSITION, &tym_i_mouseeventmask);
  mouseinterval(0);
  return 0;
error:
  return -1;
}

static int cleanup(void){
  mousemask(tym_i_mouseeventmask, 0);
  endwin();
  return 0;
}

static int pane_create(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = calloc(1,sizeof(struct curses_backend_pane));
  if(!cbp)
    goto error;
  pane->backend = cbp;
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  cbp->window = newwin(
    h, w,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer
  );
  if(!cbp->window)
    goto error_after_calloc;
  scrollok(cbp->window, true);
  leaveok(cbp->window, false);
  return 0;
error_after_calloc:
  free(cbp);
error:
  return -1;
}

static void pane_destroy(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  pane->backend = 0;
  delwin(cbp->window);
  free(cbp);
}

static int pane_refresh(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  return wrefresh(cbp->window) == OK ? 0 : -1;
}

static int pane_resize(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  unsigned x = pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned y = pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - x;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - y;
  tym_i_debug("pane_resize %ux%u %ux%u\n", x,y,w,h);
  unsigned ch=1, cw=1;
  getmaxyx(cbp->window, ch, cw);
  wresize(cbp->window, h > ch ? ch : h, w > cw ? cw : w);
  if(mvwin(cbp->window, y, x) != OK){
    tym_i_debug("mvwin(%u,%u) failed\n", y, x);
    return -1;
  }
  if(wresize(cbp->window, h, w) != OK){
    tym_i_debug("wresize(%u,%u) failed\n", h, w);
    return -1;
  }
  return 0;
}

static int pane_scroll(struct tym_i_pane_internal* pane, int n){
  struct curses_backend_pane* cbp = pane->backend;
  return wscrl(cbp->window, n) == OK ? 0 : -1;
}

static int pane_set_cursor_position(struct tym_i_pane_internal* pane, struct tym_i_cell_position position){
  struct curses_backend_pane* cbp = pane->backend;
  return wmove(cbp->window, position.y, position.x) == OK ? 0 : -1;
}

static attr_t attr2curses(enum tym_i_character_attribute ca){
  attr_t r = 0;
  if(ca & TYM_I_CA_BOLD)      r |= A_BOLD;
  if(ca & TYM_I_CA_UNDERLINE) r |= A_UNDERLINE;
  if(ca & TYM_I_CA_BLINK)     r |= A_BLINK;
  if(ca & TYM_I_CA_INVERSE)   r |= A_REVERSE;
  if(ca & TYM_I_CA_INVISIBLE) r |= A_INVIS;
  return r;
}

static int set_attribute(
  struct tym_i_pane_internal* pane,
  struct tym_i_character_format format
){
  struct curses_backend_pane* cbp = pane->backend;
  if(!has_colors())
    return -1;
  int pair = -1;
  attr_t attr = attr2curses(format.attribute);
  if(COLOR_PAIRS >= 45){
    int fi = 0;
    int bi = 0;
    if(format.fgcolor.index == 0xFF){
      // TODO
    }else if(format.fgcolor.index < 20){
      fi = format.fgcolor.index % 10;
    }
    if(format.bgcolor.index == 0xFF){
      // TODO
    }else if(format.bgcolor.index < 20){
      bi = format.bgcolor.index % 10;
    }
    if(fi > 8) fi = 0; 
    if(bi > 8) bi = 0; 
    pair = colorpair8x8_triangular_number_mirror_mapping_index[fi][bi];
    if(pair & COLORPAIR_MAPPING_NEGATIVE_FLAG)
      attr ^= A_REVERSE;
    pair &= ~COLORPAIR_MAPPING_NEGATIVE_FLAG;
  }else if(COLOR_PAIRS >= 16){
    // TODO
  }
  return wattr_set(cbp->window, attr, pair, 0);
}

static int pane_set_character(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position position,
  struct tym_i_character_format format,
  size_t length, const char utf8[length+1]
){
  struct curses_backend_pane* cbp = pane->backend;
  set_attribute(pane, format);
  wmove(cbp->window, position.y, position.x);
  waddstr(cbp->window, utf8);
  return 0;
}

int resize(void){
  tym_i_debug("resize %ux%u\n", tym_i_ttysize.ws_col, tym_i_ttysize.ws_row);
  if(resizeterm(tym_i_ttysize.ws_row, tym_i_ttysize.ws_col))
    tym_i_debug("resizeterm(%u, %u)\n",tym_i_ttysize.ws_row, tym_i_ttysize.ws_col);
  cbreak();
  nodelay(stdscr, true);
  noecho();
  keypad(stdscr, true);
  leaveok(stdscr, false);
  return 0;
}

/*
static int pane_set_cursor_mode(struct tym_i_pane_internal* pane, enum tym_i_cursor_mode cursor_mode);
static int pane_erase_area(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position start,
  struct tym_i_cell_position end,
  bool block,
  struct tym_i_character_format format
);
static int pane_set_area_to_character(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position start,
  struct tym_i_cell_position end,
  bool block,
  enum tym_i_character_attribute attribute,
  size_t length, const char utf8[length+1]
);*/

TYM_I_BACKEND_REGISTER(1000, "curses", (
  .init = init,
  .cleanup = cleanup,
  .resize = resize,
  .pane_create = pane_create,
  .pane_destroy = pane_destroy,
  .pane_resize = pane_resize,
  .pane_scroll = pane_scroll,
  .pane_refresh = pane_refresh,
  .pane_set_cursor_position = pane_set_cursor_position,
  .pane_set_character = pane_set_character,
))
