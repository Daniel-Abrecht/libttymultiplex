// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <stdio.h>
#include <unistd.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/parser.h>
#include <internal/backend.h>
#include <internal/pseudoterminal.h>

int fpipe[2] = {-1,-1};
int result = 0;

struct tym_super_position_rectangle top_pane_coordinates = {
  .edge[TYM_RECT_BOTTOM_RIGHT].type[TYM_P_RATIO].axis = {
    [TYM_AXIS_HORIZONTAL].value.real = 1,
    [TYM_AXIS_VERTICAL].value.real = 1,
  }
};

void tym_i_csq_test_hook(const struct tym_i_pane_internal* pane, int ret, const struct tym_i_command_sequence* command){
  if(ret != 0 && ret != -1)
    result = 1;
  (void)pane;
  printf("escape sequence: %d <- %s\n", ret, command->callback_name);
}

void tym_i_nocsq_test_hook(const struct tym_i_pane_internal* pane, char ch){
  uint8_t c = ch;
  (void)pane;
  if(c == 0){
    close(fpipe[1]);
    return;
  }
  if(c < 0x20 || c == 0x7F){
    printf("control character: 0x%.2x\n", (int)c);
  }else{
    printf("regular character: 0x%.2x %c\n", (int)c, (char)c);
    result = 1;
  }
}


int main(int argc, char* argv[]){
  if(argc != 2){
    fprintf(stderr, "Usage: %s sequence\n", argv[0]);
    return 1;
  }
  if(pipe(fpipe) == -1){
    perror("pipe failed");
    return 1;
  }
  if(setenv("TM_BACKEND", TYM_I_BACKEND_NAME, true) == -1){
    perror("setenv failed");
    return 1;
  }
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
  if(dprintf(fd, "%s%c", argv[1], 0) == -1)
    return -1;
  while(true){
    char c = 0;
    ssize_t ret = read(fpipe[0], &c, 1);
    if(ret == -1 && errno == EINTR)
      continue;
    if(ret){
      perror("read failed");
      return 1;
    }
    if(ret == 0)
      break;
  }
  tym_shutdown();
  return result;
}

static int update_terminal_size_information(void){
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_HORIZONTAL) = 80;
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_VERTICAL  ) = 24;
  return 0;
}

static int init(struct tym_i_backend_capabilities* caps){
  (void)caps;
  return 0;
}

static int cleanup(bool zap){
  (void)zap;
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
}

static int pane_resize(struct tym_i_pane_internal* pane){
  (void)pane;
  return 0;
}

static int pane_scroll_region(struct tym_i_pane_internal* pane, int n, unsigned top, unsigned bottom){
  (void)pane;
  (void)n;
  (void)top;
  (void)bottom;
  return 0;
}

static int pane_set_cursor_position(struct tym_i_pane_internal* pane, struct tym_i_cell_position position){
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
  (void)pane;
  (void)position;
  (void)format;
  (void)length;
  (void)utf8;
  (void)insert;
  return 0;
}

static int pane_delete_characters(struct tym_i_pane_internal* pane, struct tym_i_cell_position position, unsigned n){
  (void)pane;
  (void)position;
  (void)n;
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
