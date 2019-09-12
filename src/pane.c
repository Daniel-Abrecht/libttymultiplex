// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
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
#include <internal/parser.h>
#include <internal/pseudoterminal.h>
#include <internal/backend.h>
#include <internal/terminfo_helper.h>
#include <libttymultiplex.h>

/** \file */

struct tym_i_pane_internal *tym_i_pane_list_start, *tym_i_pane_list_end;
struct tym_i_pane_internal *tym_i_focus_pane;
const struct tym_i_character_format tym_i_default_character_format;

/** Checks if character is specified using a utf-8 sequence or using enother charset. */
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

/** Recalculate the position and size of the pane and call the reseize handlers. */
void tym_i_pane_update_size(struct tym_i_pane_internal* pane){
  tym_i_calc_rectangle_absolut_position(&pane->absolute_position, &pane->super_position);
  for(size_t i=0; i<pane->resize_handler_count; i++){
    struct tym_i_pane_resize_handler_ptr_pair* cp = pane->resize_handler_list + i;
    cp->callback(cp->ptr, pane->id, &pane->super_position, &pane->absolute_position);
  }
  tym_i_backend->pane_resize(pane);
  tym_i_backend->pane_refresh(pane);
  unsigned w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  struct winsize size = {
    .ws_col = w,
    .ws_row = h,
  };
  if(ioctl(pane->master, TIOCSWINSZ, &size) == -1)
    TYM_U_PERROR(TYM_LOG_ERROR, "ioctl TIOCSWINSZ failed");
}

/** Add a resize handler for a pane */
int tym_i_pane_resize_handler_add(struct tym_i_pane_internal* pane, const struct tym_i_pane_resize_handler_ptr_pair* cp){
  return tym_i_list_add(sizeof(*pane->resize_handler_list), &pane->resize_handler_count, (void**)&pane->resize_handler_list, cp);
}

/** Remove a resize handler for a pane */
int tym_i_pane_resize_handler_remove(struct tym_i_pane_internal* pane, size_t entry){
  return tym_i_list_remove(sizeof(*pane->resize_handler_list), &pane->resize_handler_count, (void**)&pane->resize_handler_list, entry);
}

/**
 * Add a new pane to the list of all panes
 * 
 * \see tym_pane_create
 */
void tym_i_pane_add(struct tym_i_pane_internal* pane){
  int id = 2;
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

/** Get a pane by its id */
struct tym_i_pane_internal* tym_i_pane_get(int pane){
  if(pane == TYM_PANE_FOCUS)
    return tym_i_focus_pane;
  for(struct tym_i_pane_internal* it=tym_i_pane_list_start; it; it=it->next)
    if(it->id == pane)
      return it;
  return 0;
}

/** Set the focus on a pane. THis will set tym_i_focus_pane. */
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

/** Update the visible cursor position. This is only done for the pane in focus, the other panes don't have a vidible cursor. */
void tym_i_pane_update_cursor(struct tym_i_pane_internal* pane){
  if(pane != tym_i_focus_pane || !pane)
    return;
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  tym_i_backend->pane_set_cursor_position(pane, screen->cursor);
}

/**
 * Remove a pane from the list of all panes
 * 
 * \see tym_pane_destroy
 */
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

static int pane_ptm_input_handler(void* ptr, short event, int fd){
  if(!(event & POLLIN))
    return -1;
  struct tym_i_pane_internal* pane = ptr;
  static char buf[256];
  ssize_t ret;
  do {
    ret = read(fd, buf, sizeof(buf));
  } while(ret == -1 && errno == EINTR);
  if(ret == -1)
    return -1;
  for(size_t i=0; i<(size_t)ret; i++)
    tym_i_pane_parse(pane, buf[i]);
  tym_i_backend->pane_refresh(pane);
  return 0;
}

int tym_pane_create(const struct tym_super_position_rectangle*restrict super_position){
  static const struct tym_super_position_rectangle zeropos;
  if(!super_position)
    super_position = &zeropos;
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
  struct tym_i_pane_internal* pane = tym_i_copy(sizeof(hpane), &hpane);
  pane->super_position = *super_position;
  tym_i_calc_rectangle_absolut_position(&pane->absolute_position, &pane->super_position);
  unsigned w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  struct winsize size = {
    .ws_col = w,
    .ws_row = h,
  };
  int ret = openpty(&pane->master, &pane->slave, 0, &pane->termios, &size);
  if(ret)
    goto error;
  fcntl(pane->master, F_SETFD, FD_CLOEXEC | O_NONBLOCK);
  fcntl(pane->slave, F_SETFD, FD_CLOEXEC);
  if(tym_i_backend->pane_create(pane) != 0)
    goto error;
  tym_i_pane_add(pane);
  if(tym_i_pollfd_add(pane->master, &(struct tym_i_pollfd_complement){
    .ptr = pane,
    .onevent = pane_ptm_input_handler
  })) goto error;
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

/**
 * This function sets the cursor position & handles bound checks and scrolling if necessary.
 * This functions is split into 4 steps: <br/>
 *  1) Calculate absolute desired position in the pane based on the given coordinates (x,y) and what these relate to (pm_x,pm_y) <br/>
 *  2) What are the boundaries for our cursor movement? <br/>
 *  3) Do we need to scroll the scrolling region? <br/>
 *  4) Clamp the cursor coordinates to the boundary it's allowed in & finally set the cursor position. <br/>
 * 
 * \param x The new x position
 * \param y The new y position
 * \param allow_cursor_on_right_edge If the cursor position advances due to an added character at the very right most, it won't wrap around yet. Currently, this state is stored as the cursor being one past the last character of the line, just outside the usual boundaries. It can't be placed there by escape sequences or similar means.
 **/
int tym_i_pane_set_cursor_position(
  struct tym_i_pane_internal* pane,
  enum tym_i_scp_position_mode pm_x, long long x,
  enum tym_i_scp_scrolling_mode smm_y, enum tym_i_scp_position_mode pm_y, long long y,
  enum tym_i_scp_scroll_region_behaviour srb, bool allow_cursor_on_right_edge
){
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  bool scroll_region_valid = screen->scroll_region_top < screen->scroll_region_bottom && screen->scroll_region_top < h;
  bool origin_mode = screen->origin_mode && scroll_region_valid;
  unsigned sr_top    = scroll_region_valid ? screen->scroll_region_top : 0;
  unsigned sr_bottom = scroll_region_valid ? screen->scroll_region_bottom : h;

  //// Calculating the desired position ////

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

  //// Calculate boundaries of possible cursor positions ////

  long long top = 0;
  long long bottom = h;
  long long left = 0;
  long long right = w;

  switch(srb){
    case TYM_I_SCP_SCROLLING_REGION_IRRELEVANT: break;
    case TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE: {
      if( screen->cursor.y >= sr_top && y < sr_top )
        goto set_scrolling_region_boundary;
      if( screen->cursor.y < (long long)sr_bottom && y >= (long long)sr_bottom )
        goto set_scrolling_region_boundary;
      if( screen->cursor.y < (long long)sr_bottom && screen->cursor.y >= (long long)sr_top )
        goto set_scrolling_region_boundary;
    } break;
    case TYM_I_SCP_SCROLLING_REGION_LOCKIN_IN_ORIGIN_MODE: {
      if(origin_mode)
        goto set_scrolling_region_boundary;
    } break;
    set_scrolling_region_boundary: {
      if(!scroll_region_valid)
        break;
      top = sr_top;
      bottom = sr_bottom;
    } break;
  }

  //// Do we need to scroll the scrolling region? ////

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

  //// Clamp the cursor coordinates to the allowed range & set the cursor position ////

  if(y >= bottom)
    y = bottom - 1;
  if(y < top)
    y = top;
  if(x >= right)
    x = right - !allow_cursor_on_right_edge;
  if(x < left)
    x = left;

  screen->cursor.y = y;
  screen->cursor.x = x;

  tym_i_pane_update_cursor(pane);

  return 0;
}

int tym_pane_resize(int pane, const struct tym_super_position_rectangle*restrict super_position){
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
  ppane->super_position = *super_position;
  tym_i_pane_update_size(ppane);
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

static const char* getTerm(void){

  bool found = false;

  #define VARIANTS \
    X(mouse) \
    X(color) \
    X(256color) \
    X(rgbcolor)

  const char*const term_suffixies[] = {
    "",
  #define X(Y) "-" #Y,
    VARIANTS
  #undef X
  };

  enum term_variants {
    V_NONE,
  #define X(Y) V_ ## Y,
    VARIANTS
  #undef X
    V_COUNT,
  };

  #define X(Y) + sizeof(#Y)
  enum { term_name_max_len=sizeof("libttymultiplex") VARIANTS };
  #undef X
  #undef VARIANTS

  static char term[term_name_max_len];

  bool variants[V_COUNT] = {0};
  if(tym_i_backend_capabilities->mouse)
    variants[V_mouse] = true;
  if(tym_i_backend_capabilities->color_rgb){
    variants[V_rgbcolor] = true;
    variants[V_256color] = true;
    variants[V_color] = true;
  }else if(tym_i_backend_capabilities->color_256){
    variants[V_256color] = true;
    variants[V_color] = true;
  }else if(tym_i_backend_capabilities->color_8){
    variants[V_color] = true;
  }
  static const enum term_variants check_list[][V_COUNT] = {
    { V_rgbcolor, V_mouse },
    { V_256color, V_mouse },
    { V_color, V_mouse },
    { V_rgbcolor },
    { V_256color },
    { V_color },
    { V_mouse },
    {0}
  };
  const size_t check_list_count = sizeof(check_list)/sizeof(*check_list);
  for(unsigned i=0; i<check_list_count; i++){
    strcpy(term, "libttymultiplex");
    for(unsigned j=0; j<V_COUNT; j++){
      if(!check_list[i][j])
        continue;
      if(!variants[check_list[i][j]])
        goto next;
      strcat(term, term_suffixies[check_list[i][j]]);
    }
    int res = tym_i_open_terminfo(term, O_RDONLY);
    if(res != -1){
      close(res);
      found = true;
      break;
    }
  next:;
  }

  if(!found)
    strcpy(term, "xterm");

  return term;
}

int tym_pane_get_default_env_vars(
  int pane, void* ptr,
  int(*callback)(int pane, void* ptr, size_t count, const char* env[count][2])
){
  const char* env[][2] = {
    {"TERM", getTerm()}
  };
  size_t count = sizeof(env)/sizeof(*env);
  return (*callback)(pane, ptr, count, env);
}

static int do_set_env(int pane, void* ptr, size_t count, const char* env[count][2]){
  (void)pane;
  (void)ptr;
  for(size_t i=0; i<count; i++)
    setenv(env[i][0], env[i][1], true);
  return 0;
}

int tym_pane_set_env(int pane){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED && tym_i_binit != INIT_STATE_FROZEN){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  // We are a session leader!!!
  if(getpid() == getsid(0)){
    if(ioctl(0,TIOCNOTTY) == -1){
      TYM_U_PERROR(TYM_LOG_ERROR, "ioctl(0,TIOCNOTTY) failed");
      goto error;
    }
  }
  // Let's try if this is already possible
  if(login_tty(ppane->slave) == -1){
    if(errno != EPERM){
      TYM_U_PERROR(TYM_LOG_ERROR, "login_tty failed");
      goto error;
    }
    // Maybe we just need a new session?
    if(setsid() == -1){
      // We are probably a session leader of a session/pgid which is our current controling terminal
      // We need to change to another empty and unassociated process group to fix that
      int waitfd[2];
      if(pipe(waitfd) == -1){
        TYM_U_PERROR(TYM_LOG_ERROR, "pipe failed");
        goto error;
      }
      int ret = fork(); // Create another temporary process
      if(ret == -1){
        close(waitfd[0]);
        close(waitfd[1]);
        TYM_U_PERROR(TYM_LOG_ERROR, "fork failed");
        goto error;
      }
      if(ret){
        close(waitfd[1]);
        while(read(waitfd[0], (char[]){0}, 1) == -1 && errno == EINTR);
        close(waitfd[0]);
        if(setpgid(0,ret) == -1){ // Move to process group of temporary child process
          TYM_U_PERROR(TYM_LOG_ERROR, "setpgid failed");
          // Kill temporary child process
          if(kill(ret, SIGKILL) == -1)
            TYM_U_PERROR(TYM_LOG_ERROR, "kill failed");
          goto error;
        }
        // Kill temporary child process
        if(kill(ret, SIGKILL) == -1)
          TYM_U_PERROR(TYM_LOG_ERROR, "kill failed");
        while(waitpid(ret,0,0) == -1 && errno == EINTR);
      }else{
        close(waitfd[0]);
        if(setpgid(0,0)) // Make it it's process group, create a new process group!!!
          TYM_U_PERROR(TYM_LOG_ERROR, "setpgid failed");
        close(waitfd[1]); // Signal the other process we're done
        // Let the child wait until it gets killed
        while(true)
          pause();
        abort(); // Shouldn't be reachable
      }
    }else{
      // Give up old controling terminal & processes from process group and so on so we can take a new one
      if(ioctl(0,TIOCNOTTY) == -1){
        TYM_U_PERROR(TYM_LOG_ERROR, "ioctl(0,TIOCNOTTY) failed");
        goto error;
      }
    }
    // Try again...
    if(login_tty(ppane->slave)){
      TYM_U_PERROR(TYM_LOG_ERROR, "login_tty failed");
      goto error;
    }
  }
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigprocmask(SIG_SETMASK, &sigmask, 0); // reset all signals
  tym_pane_get_default_env_vars(pane, 0, do_set_env);
  pthread_mutex_unlock(&tym_i_lock);
  return 0;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_get_slavefd(int pane){
  pthread_mutex_lock(&tym_i_lock);
  if(tym_i_binit != INIT_STATE_INITIALISED && tym_i_binit != INIT_STATE_FROZEN){
    errno = EINVAL;
    goto error;
  }
  struct tym_i_pane_internal* ppane = tym_i_pane_get(pane);
  if(!ppane){
    errno = ENOENT;
    goto error;
  }
  int fd = ppane->slave;
  pthread_mutex_unlock(&tym_i_lock);
  return fd;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

/** Scroll/Move a region of a pane and fill clear new cells. */
int tym_i_scroll_def_scrolling_region(struct tym_i_pane_internal* pane, unsigned top, unsigned bottom, int n){
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  if(bottom > h)
    bottom = h;
  if( top < bottom && !(top == 0 && bottom == h)){
    return tym_i_backend->pane_scroll_region(pane, n, top, bottom);
  }else{
    return tym_i_backend->pane_scroll(pane, n);
  }
}

/** Scroll the current panes scrolling region. */
int tym_i_scroll_scrolling_region(struct tym_i_pane_internal* pane, int n){
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  return tym_i_scroll_def_scrolling_region(pane, screen->scroll_region_top, screen->scroll_region_bottom, n);
}

/** Insert or delete some lines below a certain line. */
int tym_i_pane_insert_delete_lines(struct tym_i_pane_internal* pane, unsigned y, int n){
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  if(y >= h)
    return 0;
  return tym_i_scroll_def_scrolling_region(pane, y, h, n);
}

/** Try to swith to another screen */
int tym_i_pane_set_screen(struct tym_i_pane_internal* pane, enum tym_i_pane_screen screen){
  enum tym_i_pane_screen old = pane->current_screen;
  pane->current_screen = screen;
  int res = tym_i_backend->pane_change_screen(pane);
  if(res == -1)
    pane->current_screen = old;
  return res;
}

/** \see tym_pane_reset */
int tym_i_pane_reset(struct tym_i_pane_internal* pane){
  memset(pane->screen, 0, sizeof(pane->screen));
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  pane->mouse_mode = TYM_I_MOUSE_MODE_OFF;
  pane->character.not_utf8 = false;
  tym_i_backend->pane_erase_area(pane, (struct tym_i_cell_position){.x=0,.y=0}, (struct tym_i_cell_position){.x=w,.y=h}, false, screen->character_format);
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_ORIGIN_RELATIVE, 0,
    TYM_I_SCP_SMM_NO_SCROLLING, TYM_I_SCP_PM_ORIGIN_RELATIVE, 0,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE, false
  );
  return 0;
}

int tym_pane_reset(int pane){
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
  int ret = tym_i_pane_reset(ppane);
  pthread_mutex_unlock(&tym_i_lock);
  return ret;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_send_key(int pane, uint_least16_t key){
  int ret = 0;
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
  ret = tym_i_pts_send_key(ppane, key);
  pthread_mutex_unlock(&tym_i_lock);
  return ret;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_send_keys(int pane, size_t count, const uint_least16_t keys[count]){
  int ret = 0;
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
  ret = tym_i_pts_send_keys(ppane, count, keys);
  pthread_mutex_unlock(&tym_i_lock);
  return ret;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_type(int pane, size_t count, const char keys[count]){
  int ret = 0;
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
  ret = tym_i_pts_type(ppane, count, keys);
  pthread_mutex_unlock(&tym_i_lock);
  return ret;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}

int tym_pane_send_special_key_by_name(int pane, const char* key_name){
  size_t length = strlen(key_name);
  for(size_t i=0; i<tym_special_key_count; i++){
    if(tym_special_key_list[i].name_length != length)
      continue;
    if(memcmp(tym_special_key_list[i].name, key_name, length))
      continue;
    return tym_pane_send_key(pane, tym_special_key_list[i].key);
  }
  if(length == 1)
    return tym_pane_send_key(TYM_PANE_FOCUS, (signed char)key_name[0]);
  errno = ENOENT;
  return -1;
}

int tym_pane_send_mouse_event(int pane, enum tym_button button, const struct tym_super_position*restrict position){
  int ret = 0;
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
  struct tym_absolute_position pos;
  tym_i_calc_absolut_position(&pos, &tym_i_bounds, position, true);
  struct tym_i_cell_position cell_position = {
    .x = TYM_POS_REF(pos, CHARFIELD, TYM_AXIS_VERTICAL),
    .y = TYM_POS_REF(pos, CHARFIELD, TYM_AXIS_HORIZONTAL),
  };
  ret = tym_i_pts_send_mouse_event(ppane, button, cell_position);
  pthread_mutex_unlock(&tym_i_lock);
  return ret;
error:
  pthread_mutex_unlock(&tym_i_lock);
  return -1;
}
