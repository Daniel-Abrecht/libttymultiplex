// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <unistd.h>
#include <ncurses.h>
#include <stdlib.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/backend.h>
#include <internal/pseudoterminal.h>

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

struct curses_screen_state {
  WINDOW* window;
};

struct curses_backend_pane {
  struct curses_screen_state screen[TYM_I_SCREEN_COUNT];
};

static int terminal_input_handler(void* ptr, short event, int fd){
  (void)fd;
  if(!(event & POLLIN))
    return -1;
  (void)ptr;
  int c = getch();
  if(c != ERR)
  switch(c){
    case KEY_MOUSE: {
      MEVENT event;
      if(getmouse(&event) != OK)
        break;
      for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next){
        int left   = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_LEFT  );
        int right  = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_RIGHT );
        int top    = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_TOP   );
        int bottom = TYM_RECT_POS_REF(it->absolute_position, CHARFIELD, TYM_BOTTOM);
        if( left > (int)event.x || right <= (int)event.x || top > (int)event.y || bottom <= (int)event.y )
          continue;
        unsigned x = event.x - left;
        unsigned y = event.y - top;
        tym_i_pane_focus(it);
        if(event.bstate & (BUTTON1_RELEASED | BUTTON2_RELEASED | BUTTON3_RELEASED)){
          tym_i_pts_send_mouse_event(it, TYM_BUTTON_RELEASED, (struct tym_i_cell_position){.x=x, .y=y});
        }
        if(event.bstate & BUTTON1_PRESSED){
          tym_i_pts_send_mouse_event(it, TYM_BUTTON_LEFT_PRESSED, (struct tym_i_cell_position){.x=x, .y=y});
        }
        if(event.bstate & BUTTON2_PRESSED){
          tym_i_pts_send_mouse_event(it, TYM_BUTTON_MIDDLE_PRESSED, (struct tym_i_cell_position){.x=x, .y=y});
        }
        if(event.bstate & BUTTON3_PRESSED){
          tym_i_pts_send_mouse_event(it, TYM_BUTTON_RIGHT_PRESSED, (struct tym_i_cell_position){.x=x, .y=y});
        }
        break;
      }
    } break;
    case KEY_ENTER: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_ENTER); break;
    case KEY_UP   : tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_UP); break;
    case KEY_DOWN : tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_DOWN); break;
    case KEY_RIGHT: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_RIGHT); break;
    case KEY_LEFT : tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_LEFT); break;
    case KEY_BACKSPACE: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_BACKSPACE); break;
    case KEY_HOME: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_HOME); break;
    case KEY_END: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_END); break;
    case KEY_DC: tym_i_pts_send_key(tym_i_focus_pane, TYM_KEY_DELETE); break;
    default: tym_i_pts_send_key(tym_i_focus_pane, c); break;
  }
  return 0;
}


static int init(void){
  if(tym_i_pollfd_add(dup(STDIN_FILENO), &(struct tym_i_pollfd_complement){
    .onevent = terminal_input_handler
  })) goto error;
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
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  if(!cscreen->window) return;
}

static int pane_create(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = calloc(1,sizeof(struct curses_backend_pane));
  if(!cbp)
    goto error;
  pane->backend = cbp;
  long w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  long h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  if(h>0 && w>0){
    cbp->screen[TYM_I_SCREEN_DEFAULT].window = newpad(h, w);
    if(!cbp->screen[TYM_I_SCREEN_DEFAULT].window)
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
    if(cbp->screen[i].window)
      delwin(cbp->screen[i].window);
  free(cbp);
}

static int pane_refresh(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  if(!cscreen->window) return 0;
  int ltx = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_LEFT  );
  int lty = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_TOP   );
  int brx = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_RIGHT );
  int bry = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_BOTTOM);
  int scw = TYM_RECT_SIZE(tym_i_bounds, CHARFIELD, TYM_AXIS_HORIZONTAL);
  int sch = TYM_RECT_SIZE(tym_i_bounds, CHARFIELD, TYM_AXIS_VERTICAL);
  if(ltx >= brx || lty >= bry || brx < 0 || bry < 0 || ltx >= scw || lty >= sch)
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
  if(brx >= scw)
    brx = scw;
  if(bry >= sch)
    bry = sch - 1;
  return prefresh(cscreen->window, ofy, ofx, lty, ltx, bry, brx) == OK ? 0 : -1;
}

static int pane_resize(struct tym_i_pane_internal* pane){
  struct curses_backend_pane* cbp = pane->backend;
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  long w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  long h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  if(w <= 0 || h <= 0){
    if(cscreen->window){
      delwin(cscreen->window);
      cscreen->window = 0;
    }
    return 0;
  }
  if(!cscreen->window){
    cscreen->window = newpad(h, w);
    if(!cscreen->window){
      TYM_U_LOG(TYM_LOG_ERROR, "newpad(%ld, %ld) failed\n", h, w);
      return -1;
    }
    initpad(pane);
  }
  if(wresize(cscreen->window, h, w) != OK){
    TYM_U_LOG(TYM_LOG_ERROR, "wresize(%ld, %ld) failed\n", h, w);
    return -1;
  }
  pane_refresh(pane);
  return 0;
}

static int pane_change_screen(struct tym_i_pane_internal* pane){
  return pane_resize(pane);
}

static int pane_scroll_region(struct tym_i_pane_internal* pane, int n, unsigned top, unsigned bottom){
  if(n == 0)
    return 0;
  if(top >= bottom)
    return -1;
  struct curses_backend_pane* cbp = pane->backend;
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  long w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  long h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  if(top >= h)
    return -1;
  if(bottom > h)
    bottom = h;
  unsigned long m = bottom - top;
  TYM_U_LOG(TYM_LOG_DEBUG, "moving scrolling region: %u %u %lu %d\n",top,bottom,m,n);
  if(m <= (unsigned)abs(n)){
    return tym_i_backend->pane_erase_area(
      pane,
      (struct tym_i_cell_position){ .x=0, .y=top },
      (struct tym_i_cell_position){ .x=w, .y=bottom },
      true,
      tym_i_default_character_format
    );
  }else{
    WINDOW* region = subpad(cscreen->window, m, w, top, 0);
    if(!region)
      return -1;
    scrollok(region, true);
    int res = wscrl(region, n) == OK ? 0 : -1;
    scrollok(region, false);
    delwin(region);
    touchline(cscreen->window, top, m);
    return res;
  }
}

static int pane_scroll(struct tym_i_pane_internal* pane, int n){
  if(n == 0)
    return 0;
  struct curses_backend_pane* cbp = pane->backend;
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  scrollok(cscreen->window, true);
  int res = wscrl(cscreen->window, n) == OK ? 0 : -1;
  scrollok(cscreen->window, false);
  return res;
}

static int pane_set_cursor_position(struct tym_i_pane_internal* pane, struct tym_i_cell_position position){
  struct curses_backend_pane* cbp = pane->backend;
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  if(!cscreen->window) return 0;
  return wmove(cscreen->window, position.y, position.x) == OK ? 0 : -1;
}

static int pane_delete_characters(struct tym_i_pane_internal* pane, struct tym_i_cell_position position, unsigned n){
  struct curses_backend_pane* cbp = pane->backend;
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  while(n--)
    mvwdelch(cscreen->window, position.y, position.x);
  return 0;
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
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  if(!cscreen->window) return 0;
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
  return wattr_set(cscreen->window, attr, pair, 0);
}

static int pane_set_character(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position position,
  struct tym_i_character_format format,
  size_t length, const char utf8[length+1],
  bool insert
){
  struct curses_backend_pane* cbp = pane->backend;
  struct curses_screen_state* cscreen = &cbp->screen[pane->current_screen];
  if(!cscreen->window) return 0;
  set_attribute(pane, format);
  wmove(cscreen->window, position.y, position.x);
  if(insert){
    winsstr(cscreen->window, utf8);
  }else{
    waddstr(cscreen->window, utf8);
  }
  return 0;
}

static int resize(void){
  int scw = TYM_RECT_SIZE(tym_i_bounds, CHARFIELD, TYM_AXIS_HORIZONTAL);
  int sch = TYM_RECT_SIZE(tym_i_bounds, CHARFIELD, TYM_AXIS_VERTICAL);
  resizeterm(sch, scw);
  TYM_U_LOG(TYM_LOG_DEBUG, "resizeterm(%u, %u)\n", scw, sch);
  cbreak();
  nodelay(stdscr, true);
  noecho();
  keypad(stdscr, true);
  leaveok(stdscr, false);
  return 0;
}

static int update_terminal_size_information(void){
  struct winsize size;
  if(ioctl(STDIN_FILENO, TIOCGWINSZ, &size) == -1){
    TYM_U_PERROR(TYM_LOG_ERROR, "ioctl TIOCGWINSZ failed\n");
    return -1;
  }
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_HORIZONTAL) = size.ws_col;
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_VERTICAL) = size.ws_row;
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
  .pane_scroll_region = pane_scroll_region,
  .pane_refresh = pane_refresh,
  .pane_set_cursor_position = pane_set_cursor_position,
  .pane_delete_characters = pane_delete_characters,
  .pane_set_character = pane_set_character,
  .update_terminal_size_information = update_terminal_size_information
))
