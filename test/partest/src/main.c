// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/backend.h>
#include <internal/pseudoterminal.h>

enum colorindex {
  CI_RED,
  CI_GREEN,
  CI_BLUE
};

struct character {
  uint8_t data[TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT];
  uint8_t fgcolor[3];
  uint8_t bgcolor[3];
  uint8_t format; // see enum tym_i_character_attribute
};

struct faketerminal {
  struct tym_i_cell_position size;
  void* content;
};

struct faketerminal terminal = {
  .size = { 80, 24 }
};

struct tym_super_position_rectangle top_pane_coordinates = {
  .edge[TYM_RECT_BOTTOM_RIGHT].type[TYM_P_RATIO].axis = {
    [TYM_AXIS_HORIZONTAL].value.real = 1,
    [TYM_AXIS_VERTICAL].value.real = 1,
  }
};

const char* dump_target = 0;
int dump_screen(void){
  uint8_t di = 0;
  static char buf[256] = {0};
  size_t len = strlen(dump_target);
  if(len > sizeof(buf)-9)
    return 1;
  ssize_t n = snprintf(buf, sizeof(buf)-1, "%s%u.", dump_target, (unsigned)di++);
  if(n < 0 || n > (ssize_t)sizeof(buf)-4)
    return 1;
  enum {
    DT_SPC,
    DT_TXT,
    DT_FMT,
    DT_COUNT
  };
  FILE* fds[DT_COUNT] = {0};
  char* sfxs[DT_COUNT] = {
    "spc",
    "txt",
    "fmt"
  };
  for(int i=0; i<DT_COUNT; i++){
    strcpy(buf+n, sfxs[i]);
    FILE* fd = fopen(buf, "wb");
    if(!fd)
      goto error;
    fds[i] = fd;
  }
  fprintf(fds[DT_SPC], "col=%d\nrow=%d\n", terminal.size.x, terminal.size.y);
  {
    struct character (*tch)[terminal.size.y][terminal.size.x] = terminal.content;
    for(size_t y=0; y<terminal.size.y; y++)
    for(size_t x=0; x<terminal.size.x; x++){
      fwrite( (*tch)[y][x].data   , 1, TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT, fds[DT_TXT]);
      fwrite( (*tch)[y][x].fgcolor, 1, 3, fds[DT_FMT]);
      fwrite( (*tch)[y][x].bgcolor, 1, 3, fds[DT_FMT]);
      fwrite(&(*tch)[y][x].format , 1, 1, fds[DT_FMT]);
    }
  }
  for(int i=0; i<DT_COUNT; i++)
    if(fds[i])
      fclose(fds[i]);
  return 0;
error:
  for(int i=0; i<DT_COUNT; i++)
    if(fds[i])
      fclose(fds[i]);
  return 1;
}

int main(int argc, char* argv[]){
  if(argc != 2){
    fprintf(stderr, "Usage: %s outputfile\n\nNote: There'll be some suffixes added to dumpfile, and there are more than one\n", argv[0]);
    return 1;
  }
  dump_target = argv[1];
  setenv("TM_BACKEND", TYM_I_BACKEND_NAME, true);
  if(tym_init()){
    perror("tym_init failed");
    return 1;
  }
  int top_pane = tym_pane_create(&top_pane_coordinates);
  if(top_pane == -1){
    perror("tym_create_pane failed");
    return 1;
  }
  if(tym_pane_set_flag(top_pane, TYM_PF_FOCUS, true) == -1){
    perror("tym_pane_focus failed");
    return 1;
  }
  int fd = tym_pane_get_slavefd(top_pane);
  int c;
  while((c=getchar()) != EOF && c != -1){
    if(c == 0){
      if(dump_screen()){
        perror("dump_screen failed");
        return 1;
      }
      continue;
    }
    if(write(fd, (char[]){c}, 1) != 1){
      perror("write failed");
      return 1;
    }
  }
  if(dump_screen()){
    perror("dump_screen failed");
    return 1;
  }
  tym_shutdown();
  return 0;
}

static int update_terminal_size_information(void){
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_HORIZONTAL) = terminal.size.x;
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_VERTICAL  ) = terminal.size.y;
  return 0;
}

static int init(void){
  terminal.content = calloc(terminal.size.x * terminal.size.y, sizeof(struct character));
  if(!terminal.content)
    return 1;
  return 0;
}

static int cleanup(void){
  return 0;
}

static int resize(void){
  return 0;
}

static int pane_create(struct tym_i_pane_internal* pane){
  (void)pane;
  return 0;
}

static void pane_destroy(struct tym_i_pane_internal* pane){
  (void)pane;
  if(terminal.content)
    free(terminal.content);
  terminal.content = 0;
}

static int pane_resize(struct tym_i_pane_internal* pane){
  (void)pane;
  return 0;
}

static int pane_scroll_region(struct tym_i_pane_internal* pane, int n, unsigned top, unsigned bottom){
  unsigned pt = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_TOP);
  unsigned pb = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_BOTTOM);
  unsigned pl = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_LEFT);
  unsigned pr = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_RIGHT);
  assert(pl <= pr);
  assert(pt <= pb);
  assert(pr <= terminal.size.x);
  assert(pb <= terminal.size.y);
  assert(top <= bottom);
  assert(bottom <= pb-pt);
  if(n == 0 || bottom-top == 0 || pr-pl == 0)
    return 0;
  if(bottom-top <= (unsigned)abs(n)){
    return tym_i_backend->pane_erase_area(
      pane,
      (struct tym_i_cell_position){ .x=0, .y=top },
      (struct tym_i_cell_position){ .x=pr-pl, .y=bottom },
      true,
      tym_i_default_character_format
    );
  }else{
    struct character (*tch)[terminal.size.y][terminal.size.x] = terminal.content;
    if(n > 0){
      for(int i=0,m=bottom-top-n; i<m; i++)
        memcpy( (*tch)[pt+top+i]+pl, (*tch)[pt+top+n+i]+pl, (pr-pl) * sizeof(struct character) );
      return tym_i_backend->pane_erase_area(
        pane,
        (struct tym_i_cell_position){ .x=0, .y=bottom-n },
        (struct tym_i_cell_position){ .x=pr-pl, .y=bottom-1 },
        true,
        tym_i_default_character_format
      );
    }else{
      for(int i=0,m=bottom-top+n; i<m; i++)
        memcpy( (*tch)[pt+bottom-i]+pl, (*tch)[pt+bottom+n-i]+pl, (pr-pl) * sizeof(struct character) );
      return tym_i_backend->pane_erase_area(
        pane,
        (struct tym_i_cell_position){ .x=0, .y=top },
        (struct tym_i_cell_position){ .x=pr-pl, .y=top-(n+1) },
        true,
        tym_i_default_character_format
      );
    }
  }
  assert(false);
  return 0;
}

static int pane_set_cursor_position(struct tym_i_pane_internal* pane, struct tym_i_cell_position position){
  unsigned pt = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_TOP);
  unsigned pb = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_BOTTOM);
  unsigned pl = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_LEFT);
  unsigned pr = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_RIGHT);
  assert(pl <= pr);
  assert(pt <= pb);
  assert(pr <= terminal.size.x);
  assert(pb <= terminal.size.y);
  assert(position.x < pr-pl);
  assert(position.y < pb-pt);
  (void)pane;
  (void)position;
  return 0;
}

static int pane_set_character(
  struct tym_i_pane_internal* pane,
  struct tym_i_cell_position position,
  struct tym_i_character_format format,
  size_t length, const char utf8[length+1],
  bool insert
){
  unsigned pt = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_TOP);
  unsigned pb = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_BOTTOM);
  unsigned pl = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_LEFT);
  unsigned pr = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_RIGHT);
  assert(pl <= pr);
  assert(pt <= pb);
  assert(pr <= terminal.size.x);
  assert(pb <= terminal.size.y);
  assert(position.x < pr-pl);
  assert(position.y < pb-pt);
  struct character (*tch)[terminal.size.y][terminal.size.x] = terminal.content;
  struct character* ch = &(*tch)[pt+position.y][pl+position.x];
  ch->format = format.attribute;
  ch->fgcolor[CI_RED]   = format.fgcolor.red;
  ch->fgcolor[CI_GREEN] = format.fgcolor.green;
  ch->fgcolor[CI_BLUE]  = format.fgcolor.blue;
  ch->bgcolor[CI_RED]   = format.bgcolor.red;
  ch->bgcolor[CI_GREEN] = format.bgcolor.green;
  ch->bgcolor[CI_BLUE]  = format.bgcolor.blue;
  if(insert)
    if(position.x+1 < pr-pl)
      memmove( (*tch)[pt+position.y]+pl+position.x, (*tch)[pt+position.y]+pl+position.x+1, (pr-pl-position.x-1) * sizeof(struct character) );
  if(length && length <= TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT)
    memcpy(ch->data, utf8, length);
  if(length < TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT)
    memset(ch->data+length, 0, TYM_I_UTF8_CHARACTER_MAX_BYTE_COUNT-length);
  return 0;
}

static int pane_delete_characters(struct tym_i_pane_internal* pane, struct tym_i_cell_position position, unsigned n){
  unsigned pt = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_TOP);
  unsigned pb = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_BOTTOM);
  unsigned pl = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_LEFT);
  unsigned pr = TYM_RECT_POS_REF(pane->absolute_position, CHARFIELD, TYM_RIGHT);
  assert(pl <= pr);
  assert(pt <= pb);
  assert(pr <= terminal.size.x);
  assert(pb <= terminal.size.y);
  assert(position.x < pr-pl);
  assert(position.y < pb-pt);
  if(pr-pl - position.x < n)
    n = pr-pl - position.x;
  if(!n)
    return 0;
  struct character (*tch)[terminal.size.y][terminal.size.x] = terminal.content;
  if(position.x+n < pr-pl)
    memmove( (*tch)[pt+position.y]+pl+position.x+n, (*tch)[pt+position.y]+pl+position.x, (pr-pl-position.x-n) * sizeof(struct character) );
  memset( (*tch)[pt+position.y]+pl+position.x, 0, n * sizeof(struct character) );
  return 0;
}

TYM_I_BACKEND_REGISTER((
  .init = init,
  .cleanup = cleanup,
  .resize = resize,
  .pane_create = pane_create,
  .pane_destroy = pane_destroy,
  .pane_resize = pane_resize,
  .pane_scroll_region = pane_scroll_region,
  .pane_set_cursor_position = pane_set_cursor_position,
  .pane_set_character = pane_set_character,
  .pane_delete_characters = pane_delete_characters,
  .update_terminal_size_information = update_terminal_size_information
))
