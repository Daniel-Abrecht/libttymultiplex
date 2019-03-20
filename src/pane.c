// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <utmp.h>
#include <fcntl.h>
#include <errno.h>
#include <internal/pane.h>
#include <internal/calc.h>
#include <internal/main.h>
#include <internal/list.h>
#include <internal/utils.h>
#include <libttymultiplex.h>

struct tym_i_pane_internal *tym_i_pane_list_start, *tym_i_pane_list_end;
struct tym_i_pane_internal *tym_i_focus_pane;

void tym_i_pane_update_size(struct tym_i_pane_internal* pane){
  tym_i_calc_absolut_position(&pane->coordinates, &pane->superposition);
  for(size_t i=0; i<pane->resize_handler_count; i++){
    struct tym_i_handler_ptr_pair* cp = pane->resize_handler_list + i;
    cp->callback(cp->ptr, pane->id, &pane->superposition, &pane->coordinates);
  }
  struct winsize size = {
    .ws_col = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer,
    .ws_row = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer,
  };
  derwin(pane->window,
    size.ws_row,
    size.ws_col,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer
  );
  wrefresh(pane->window);
  ioctl(pane->master, TIOCSWINSZ, &size);
/*
  printf("%d: x(%u-%u) y(%u-%u)\n", pane->id,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer,
    pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer,
    pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer
  );
*/
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
    wrefresh(pane->window);
  }else{
    tym_i_focus_pane = 0;
  }
  return 0;
}

void tym_i_pane_update_cursor(struct tym_i_pane_internal* pane){
  if(pane != tym_i_focus_pane || !pane)
    return;
  wmove(pane->window, pane->cursor.y, pane->cursor.x);
}

void tym_i_pane_update_size_all(void){
  if(ioctl(tym_i_tty, TIOCGWINSZ, &tym_i_ttysize) == -1){
    perror("ioctl TIOCGWINSZ failed\n");
    abort();
  }
  resizeterm(tym_i_ttysize.ws_row, tym_i_ttysize.ws_col);
  cbreak();
  nodelay(stdscr, true);
  noecho();
  keypad(stdscr, true);
  leaveok(stdscr, false);
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
  static const struct tym_superposition zeropos = {0};
  if(!superposition)
    superposition = &zeropos;
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* pane = TYM_COPY(((struct tym_i_pane_internal){
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
  }));
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
  pane->window = newwin(
    size.ws_row,
    size.ws_col,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer,
    pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer
  );
  scrollok(pane->window, true);
  leaveok(pane->window, false);
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
  delwin(ppane->window);
  ppane->window = 0;
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

void tym_i_pane_cursor_set_cursor(struct tym_i_pane_internal* pane, unsigned x, unsigned y){
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  if(x >= w)
    x = w-1;
  if(y >= h){ // TODO: scroll down on y >= h
    wscrl(pane->window, y - h + 1);
    y = h-1;
  }
  pane->cursor.y = y;
  pane->cursor.x = x;
  wmove(pane->window, y, x);
  return;
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
/*  unsigned w = ppane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - ppane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = ppane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - ppane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  char buf[10];
  snprintf(buf, sizeof(buf), "%u", w);
  setenv("LINES", buf, true);
  snprintf(buf, sizeof(buf), "%u", h);
  setenv("COLUMNS", buf, true);*/
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
