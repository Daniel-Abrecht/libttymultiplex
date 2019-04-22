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
  WINDOW* window[TYM_I_SCREEN_COUNT];
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

static void initpad(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  WINDOW* w = cbp->window[pane->current_screen];
  if(!w) return;
}

static int pane_create(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = calloc(1,sizeof(struct curses_backend_pane));
  if(!cbp)
    goto error;
  pane->backend = cbp;
  long w = (long)pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  long h = (long)pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  if(h>0 && w>0){
    cbp->window[TYM_I_SCREEN_DEFAULT] = newpad(h, w);
    if(!cbp->window[TYM_I_SCREEN_DEFAULT])
      goto error_after_calloc;
    initpad(pane);
  }
  return 0;
error_after_calloc:
  free(cbp);
error:
  return -1;
}

static void pane_destroy(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  pane->backend = 0;
  for(int i=0; i<TYM_I_SCREEN_COUNT; i++)
    if(cbp->window[i])
      delwin(cbp->window[i]);
  free(cbp);
}

static int pane_refresh(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  WINDOW* w = cbp->window[pane->current_screen];
  if(!w) return 0;
  int ltx = pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  int lty = pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  int brx = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer;
  int bry = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer;
  if(ltx >= brx || lty >= bry || brx < 0 || bry < 0 || ltx >= tym_i_ttysize.ws_col || lty >= tym_i_ttysize.ws_row)
    return -1;
  int ofx = 0;
  int ofy = 0;
  if(ltx < 0){
    ofx = -ltx;
    ltx = 0;
  }
  if(lty < 0){
    ofy = -lty;
    lty = 0;
  }
  if(brx >= tym_i_ttysize.ws_col)
    brx = tym_i_ttysize.ws_col - 1;
  if(bry >= tym_i_ttysize.ws_row)
    bry = tym_i_ttysize.ws_row - 1;
  return prefresh(w, ofy, ofx, lty, ltx, bry, brx) == OK ? 0 : -1;
}

static int pane_resize(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  int x = pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  int y = pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  long w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - x;
  long h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - y;
  if(w <= 0 || h <= 0){
    if(cbp->window[pane->current_screen]){
      delwin(cbp->window[pane->current_screen]);
      cbp->window[pane->current_screen] = 0;
    }
    return 0;
  }
  if(!cbp->window[pane->current_screen]){
    cbp->window[pane->current_screen] = newpad(h, w);
    if(!cbp->window[pane->current_screen]){
      tym_i_debug("newpad(%l, %l) failed\n", h, w);
      return -1;
    }
    initpad(pane);
  }
  if(wresize(cbp->window[pane->current_screen], h, w) != OK){
    tym_i_debug("wresize(%u, %u) failed\n", h, w);
    return -1;
  }
  pane_refresh(pane);
  return 0;
}

static int pane_change_screen(struct tym_i_pane_internal* pane){
  return pane_resize(pane);
}

static int pane_scroll(struct tym_i_pane_internal* pane, int n){
  struct curses_backend_pane* cbp = pane->backend;
  WINDOW* w = cbp->window[pane->current_screen];
  if(!w) return 0;
  scrollok(w, true);
  int res = wscrl(w, n) == OK ? 0 : -1;
  scrollok(w, false);
  return res;
}

static int pane_set_cursor_position(struct tym_i_pane_internal* pane, struct tym_i_cell_position position){
  struct curses_backend_pane* cbp = pane->backend;
  WINDOW* w = cbp->window[pane->current_screen];
  if(!w) return 0;
  return wmove(w, position.y, position.x) == OK ? 0 : -1;
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
  WINDOW* w = cbp->window[pane->current_screen];
  if(!w) return 0;
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
  return wattr_set(w, attr, pair, 0);
}

static int pane_set_character(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position position,
  struct tym_i_character_format format,
  size_t length, const char utf8[length+1]
){
  struct curses_backend_pane* cbp = pane->backend;
  WINDOW* w = cbp->window[pane->current_screen];
  if(!w) return 0;
  set_attribute(pane, format);
  wmove(w, position.y, position.x);
  waddstr(w, utf8);
  return 0;
}

int resize(void){
  if(resizeterm(tym_i_ttysize.ws_row, tym_i_ttysize.ws_col))
    tym_i_debug("resizeterm(%u, %u)\n",tym_i_ttysize.ws_row, tym_i_ttysize.ws_col);
  cbreak();
  nodelay(stdscr, true);
  noecho();
  keypad(stdscr, true);
  leaveok(stdscr, false);
  return 0;
}

TYM_I_BACKEND_REGISTER(1000, "curses", (
  .init = init,
  .cleanup = cleanup,
  .resize = resize,
  .pane_create = pane_create,
  .pane_destroy = pane_destroy,
  .pane_resize = pane_resize,
  .pane_change_screen = pane_change_screen,
  .pane_scroll = pane_scroll,
  .pane_refresh = pane_refresh,
  .pane_set_cursor_position = pane_set_cursor_position,
  .pane_set_character = pane_set_character,
))
