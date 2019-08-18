#include <stdio.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/backend.h>
#include <internal/pseudoterminal.h>

struct tym_super_position_rectangle top_pane_coordinates = {
  .edge[TYM_RECT_BOTTOM_RIGHT].type[TYM_P_RATIO].axis = {
    [TYM_AXIS_HORIZONTAL].value.real = 1,
    [TYM_AXIS_VERTICAL].value.real = 1,
  }
};

int main(int argc, char* argv[]){
  if(argc != 2){
    fprintf(stderr, "Usage: %s sequence\n", argv[0]);
    return 1;
  }
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
  char* seq = argv[1];
  for( ; *seq; seq++ ){
    if(tym_pane_send_key(TYM_PANE_FOCUS, *seq)){
      perror("tym_pane_send_key failed");
      return 1;
    }
  }
  tym_shutdown();
  return 0;
}

static int update_terminal_size_information(void){
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_HORIZONTAL) = 80;
  TYM_POS_REF(tym_i_bounds.edge[TYM_RECT_BOTTOM_RIGHT], CHARFIELD, TYM_AXIS_VERTICAL  ) = 24;
  return 0;
}

static int init(void){
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
