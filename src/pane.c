// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <utmp.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <internal/pane.h>
#include <internal/calc.h>
#include <internal/main.h>
#include <internal/list.h>
#include <internal/utils.h>
#include <internal/backend.h>
#include <libttymultiplex.h>

struct tym_i_pane_internal *tym_i_pane_list_start, *tym_i_pane_list_end;
struct tym_i_pane_internal *tym_i_focus_pane;
const struct tym_i_character_format default_character_format;

bool tym_i_character_is_utf8(struct tym_i_character character){
  uint8_t gl = character.charset_selection;
  uint8_t gr = character.charset_selection >> 8;
  if(character.not_utf8)
    return false;
  if(gl != 0 || gr != 0)
    return false;
  if(character.charset_g[gl] || character.charset_g[gr])
    return false;
  return true;
}

void tym_i_pane_update_size(struct tym_i_pane_internal* pane){
  tym_i_calc_absolut_position(&pane->coordinates, &pane->superposition);
  for(size_t i=0; i<pane->resize_handler_count; i++){
    struct tym_i_handler_ptr_pair* cp = pane->resize_handler_list + i;
    cp->callback(cp->ptr, pane->id, &pane->superposition, &pane->coordinates);
  }
  tym_i_backend->pane_resize(pane);
  tym_i_backend->pane_refresh(pane);
  struct winsize size = {
    .ws_col = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer,
    .ws_row = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer,
  };
  if(ioctl(pane->master, TIOCSWINSZ, &size) == -1)
    tym_i_perror("ioctl TIOCSWINSZ failed");
}

int tym_i_pane_resize_handler_add(struct tym_i_pane_internal* pane, const struct tym_i_handler_ptr_pair* cp){
  return tym_i_list_add(sizeof(*pane->resize_handler_list), &pane->resize_handler_count, (void**)&pane->resize_handler_list, cp);
}

int tym_i_pane_resize_handler_remove(struct tym_i_pane_internal* pane, size_t entry){
  return tym_i_list_remove(sizeof(*pane->resize_handler_list), &pane->resize_handler_count, (void**)&pane->resize_handler_list, entry);
}

void tym_i_pane_add(struct tym_i_pane_internal* pane){
  int id = 1;
  pane->previous = tym_i_pane_list_start;
  if(!tym_i_pane_list_start)
    tym_i_pane_list_start = pane;
  if(tym_i_pane_list_end){
    tym_i_pane_list_end->next = pane;
    id = tym_i_pane_list_end->id + 1;
  }
  pane->next = 0;
  pane->id = id;
  tym_i_pane_list_end = pane;
}

struct tym_i_pane_internal* tym_i_pane_get(int pane){
  for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next)
    if(it->id == pane)
      return it;
  return 0;
}

int tym_i_pane_focus(struct tym_i_pane_internal* pane){
  if(pane){
    if(pane->nofocus){
      errno = EPERM;
      return -1;
    }
    if(tym_i_focus_pane == pane)
      return 0;
    tym_i_focus_pane = pane;
    tym_i_pane_update_cursor(pane);
    tym_i_backend->pane_refresh(pane);
  }else{
    tym_i_focus_pane = 0;
  }
  return 0;
}

void tym_i_pane_update_cursor(struct tym_i_pane_internal* pane){
  if(pane != tym_i_focus_pane || !pane)
    return;
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  tym_i_backend->pane_set_cursor_position(pane, screen->cursor);
}

void tym_i_pane_update_size_all(void){
  if(ioctl(tym_i_tty, TIOCGWINSZ, &tym_i_ttysize) == -1){
    perror("ioctl TIOCGWINSZ failed\n");
    abort();
  }
  tym_i_backend->resize();
  for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next)
    tym_i_pane_update_size(it);
}

void tym_i_pane_remove(struct tym_i_pane_internal* pane){
  if(tym_i_focus_pane == pane)
    tym_i_pane_focus(0);
  if(tym_i_pane_list_start == pane)
    tym_i_pane_list_start = pane->next;
  if(tym_i_pane_list_end == pane)
    tym_i_pane_list_end = pane->previous;
  if(pane->previous)
    pane->previous->next = pane->next;
  if(pane->next)
    pane->next->previous = pane->previous;
}

int tym_pane_create(const struct tym_superposition*restrict superposition){
  static const struct tym_superposition zeropos;
  if(!superposition)
    superposition = &zeropos;
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal hpane = {
    .master = -1,
    .slave = -1,
    // Set some generic termios defaults
    .termios = {
      .c_iflag = BRKINT | ICRNL | IMAXBEL | IUTF8,
      .c_oflag = OPOST | ONLCR | NL0 | CR0 | TAB0 | BS0 | FF0,
      .c_cflag = CREAD,
      .c_lflag = ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ISIG | ECHOCTL | ECHOKE,
      .c_cc = {
#ifdef VDISCARD
        [VDISCARD] =  017,
#endif
#ifdef VDSUSP
        [VDSUSP]   =  031,
#endif
#ifdef VEOF
        [VEOF]     =   04,
#endif
#ifdef VEOL
        [VEOL]     =    0,
#endif
#ifdef VEOL2
        [VEOL2]    =    0,
#endif
#ifdef VERASE
        [VERASE]   = 0177,
#endif
#ifdef VINTR
        [VINTR]    =  003,
#endif
#ifdef VKILL
        [VKILL]    =  025,
#endif
#ifdef VLNEXT
        [VLNEXT]   =  026,
#endif
#ifdef VQUIT
        [VQUIT]    =  034,
#endif
#ifdef VREPRINT
        [VREPRINT] =  022,
#endif
#ifdef VSTART
        [VSTART]   =  021,
#endif
#ifdef VSTATUS
        [VSTATUS]  =  024,
#endif
#ifdef VSTOP
        [VSTOP]    =  023,
#endif
#ifdef VSUSP
        [VSUSP]    =  032,
#endif
#ifdef VSWTCH
        [VSWTCH]   =    0,
#endif
#ifdef VWERASE
        [VWERASE]  =  027,
#endif
      }
    },
    .sequence.seq_opt_max = -1
  };
  struct tym_i_pane_internal* pane = TYM_COPY((hpane));
  pane->superposition = *superposition;
  tym_i_calc_absolut_position(&pane->coordinates, &pane->superposition);
  struct winsize size = {0};
  size.ws_col = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  size.ws_row = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  int ret = openpty(&pane->master, &pane->slave, 0, &pane->termios, &size);
  if(ret)
    goto error;
  fcntl(pane->master, F_SETFD, FD_CLOEXEC | O_NONBLOCK);
  fcntl(pane->slave, F_SETFD, FD_CLOEXEC);
  if(tym_i_backend->pane_create(pane) != 0)
    goto error;
  tym_i_pane_add(pane);
  if(tym_i_pollfd_add(pane->master))
    goto error;
  tym_i_pane_update_size(pane);
  pthread_mutex_unlock(&tym_i_lock);
  return pane->id;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_destroy(int pane){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  tym_pane_reset(pane);
  tym_i_backend->pane_destroy(ppane);
  tym_i_pollfd_remove(ppane->master);
  tym_i_pane_remove(ppane);
  close(ppane->slave);
  free(ppane);
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_i_pane_set_cursor_position(
  struct tym_i_pane_internal* pane,
  enum tym_i_scp_position_mode pm_x, long long x,
  enum tym_i_scp_scrolling_mode smm_y, enum tym_i_scp_position_mode pm_y, long long y,
  enum tym_i_scp_scroll_region_behaviour srb
){
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  bool scroll_region_valid = screen->scroll_region_top < screen->scroll_region_bottom && screen->scroll_region_top < h;
  bool origin_mode = screen->origin_mode && scroll_region_valid;
  unsigned sr_top    = scroll_region_valid ? screen->scroll_region_top : 0;
  unsigned sr_bottom = scroll_region_valid ? screen->scroll_region_bottom : h;

  switch(pm_x){
    case TYM_I_SCP_PM_ABSOLUTE: break;
    case TYM_I_SCP_PM_RELATIVE: x += screen->cursor.x; break;
    case TYM_I_SCP_PM_ORIGIN_RELATIVE: break;
  }

  switch(pm_y){
    case TYM_I_SCP_PM_ABSOLUTE: break;
    case TYM_I_SCP_PM_RELATIVE: y += screen->cursor.y; break;
    case TYM_I_SCP_PM_ORIGIN_RELATIVE: {
      if(origin_mode)
        y += screen->scroll_region_top;
    } break;
  }

  long long top = 0;
  long long bottom = h;
  long long left = 0;
  long long right = w;

  switch(srb){
    case TYM_I_SCP_SCROLLING_REGION_IRRELEVANT: break;
    case TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE: {
      if( screen->cursor.y >= sr_top && y < sr_top )
        goto set_scrolling_region_boundary;
      if( screen->cursor.y < (long long)sr_bottom && y <= (long long)sr_bottom )
        goto set_scrolling_region_boundary;
      if( screen->cursor.y < (long long)sr_bottom && screen->cursor.y >= (long long)sr_top )
        goto set_scrolling_region_boundary;
    } break;
    case TYM_I_SCP_SCROLLING_REGION_LOCKIN_IN_ORIGIN_MODE:
      if(origin_mode)
        goto set_scrolling_region_boundary;
      break;
    set_scrolling_region_boundary: {
      if(!scroll_region_valid)
        break;
      top = sr_top;
      bottom = sr_bottom;
    } break;
  }

  switch(smm_y){
    case TYM_I_SCP_SMM_NO_SCROLLING: break;
    case TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY: {
      if(y >= bottom)
        tym_i_scroll_def_scrolling_region(pane, top, bottom, y - bottom + 1);
    } break;
    case TYM_I_SCP_SMM_SCROLL_BACKWARD_ONLY: {
      if(y < top)
        tym_i_scroll_def_scrolling_region(pane, top, bottom, y - top);
    } break;
    case TYM_I_SCP_SMM_UNRESTRICTED_SCROLLING: {
      if(y < top)
        tym_i_scroll_def_scrolling_region(pane, top, bottom, y - top);
      if(y >= bottom)
        tym_i_scroll_def_scrolling_region(pane, top, bottom, y - bottom + 1);
    }; break;
  }

  if(y >= bottom)
    y = bottom - 1;
  if(y < top)
    y = top;
  if(x >= right)
    x = right - 1;
  if(x < left)
    x = left;

  screen->cursor.y = y;
  screen->cursor.x = x;
  tym_i_pane_update_cursor(pane);
  return 0;
}

int tym_pane_resize(int pane, const struct tym_superposition*restrict superposition){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  ppane->superposition = *superposition;
  tym_i_pane_update_size(ppane);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_set_env(int pane){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  login_tty(ppane->slave);
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigprocmask(SIG_SETMASK, &sigmask, 0); // reset all signals
  setenv("TERM", "xterm", true);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_get_slavefd(int pane){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  pthread_mutex_unlock(&tym_i_lock);
  return ppane->slave;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_get_masterfd(int pane){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  pthread_mutex_unlock(&tym_i_lock);
  return ppane->master;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_i_scroll_def_scrolling_region(struct tym_i_pane_internal* pane, unsigned top, unsigned bottom, int n){
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  if(bottom > h)
    bottom = h;
  if( top < bottom && !(top == 0 && bottom == h)){
    return tym_i_backend->pane_scroll_region(pane, n, top, bottom);
  }else{
    return tym_i_backend->pane_scroll(pane, n);
  }
}

int tym_i_scroll_scrolling_region(struct tym_i_pane_internal* pane, int n){
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  return tym_i_scroll_def_scrolling_region(pane, screen->scroll_region_top, screen->scroll_region_bottom, n);
}

int tym_i_pane_set_screen(struct tym_i_pane_internal* pane, enum tym_i_pane_screen screen){
  enum tym_i_pane_screen old = pane->current_screen;
  pane->current_screen = screen;
  int res = tym_i_backend->pane_change_screen(pane);
  if(res == -1)
    pane->current_screen = old;
  return res;
}

int tym_pane_reset(int pane){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  (void)pane;
  errno = ENOSYS;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}
