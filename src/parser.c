// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <internal/main.h>
#include <internal/pane.h>
#include <internal/parser.h>

/* VT 52
  CSQ( "^=", enter_alternate_keypad_mode ) \
  CSQ( "^>", exit_alternate_keypad_mode ) \
  CSQ( "^<", exit_vt52_mode ) \
  CSQ( "^F", enter_graphics_mode ) \
  CSQ( "^G", exit_graphics_mode ) \
  CSQ( "^A", cursor_up ) \
  CSQ( "^B", cursor_down ) \
  CSQ( "^C", cursor_right ) \
  CSQ( "^D", cursor_left ) \
*/

#define CSQS \
  CSQ( RIS, reset ) \
  CSQ( ESC "(" C, designate_g0_character_set ) \
  CSQ( ESC ")" C, designate_g1_character_set ) \
  CSQ( ESC "*" C, designate_g2_character_set ) \
  CSQ( ESC "+" C, designate_g3_character_set ) \
  CSQ( ESC "7", save_cursor_position ) \
  CSQ( ESC "8", restore_cursor_position ) \
  CSQ( ESC "=", application_keypad ) \
  CSQ( ESC ">", normal_keypad ) \
  CSQ( ESC "F", cursor_to_bottom_left ) \
  CSQ( ESC "l", memory_lock ) \
  CSQ( ESC "m", memory_unlock ) \
  CSQ( ESC "n", invoke_charset_G2_as_GL_SL2 ) \
  CSQ( ESC "o", invoke_charset_G3_as_GL_SL3 ) \
  CSQ( ESC "|", invoke_charset_G3_as_GR_LS3R ) \
  CSQ( ESC "}", invoke_charset_G4_as_GR_LS2R ) \
  CSQ( ESC "~", invoke_charset_G1_as_GR_LS1R ) \
  CSQ( CSI "r", set_scrolling_region ) \
  CSQ( CSI NUM "r", set_scrolling_region ) \
  CSQ( CSI "!p", soft_reset ) \
  CSQ( CSI NUM "l", reset_mode ) \
  CSQ( CSI "c", send_device_attributes_primary ) \
  CSQ( CSI NUM "c", send_device_attributes_primary ) \
  CSQ( CSI "e", vertical_position_relative /* VPR aka. line position forward */ ) \
  CSQ( CSI NUM "e", vertical_position_relative ) \
  CSQ( CSI "a", character_position_relative /* VPR aka. character position forward */ ) \
  CSQ( CSI NUM "a", character_position_relative ) \
  CSQ( CSI "k", vertical_position_backwards /* VPB */ ) \
  CSQ( CSI NUM "k", vertical_position_backwards ) \
  CSQ( CSI "A", cursor_up ) \
  CSQ( CSI NUM "A", cursor_up ) \
  CSQ( CSI "B", cursor_down ) \
  CSQ( CSI NUM "B", cursor_down ) \
  CSQ( CSI "C", cursor_right ) \
  CSQ( CSI NUM "C", cursor_right ) \
  CSQ( CSI "D", cursor_left ) \
  CSQ( CSI NUM "D", cursor_left ) \
  CSQ( CSI "E", cursor_next_line ) \
  CSQ( CSI NUM "E", cursor_next_line ) \
  CSQ( CSI "F", cursor_previous_line ) \
  CSQ( CSI NUM "F", cursor_previous_line ) \
  CSQ( CSI "G", cursor_horizontal_absolute ) \
  CSQ( CSI NUM "G", cursor_horizontal_absolute ) \
  CSQ( CSI "H", cursor_position ) \
  CSQ( CSI NUM "H", cursor_position ) \
  CSQ( CSI "?" NUM "c", erase_in_display ) \
  CSQ( CSI "J", erase_in_display ) \
  CSQ( CSI "?" "J", erase_in_display ) \
  CSQ( CSI NUM "J", erase_in_display ) \
  CSQ( CSI "?" NUM "J", erase_in_display ) \
  CSQ( CSI "K", erase_in_line ) \
  CSQ( CSI "?" "K", erase_in_line ) \
  CSQ( CSI NUM "K", erase_in_line ) \
  CSQ( CSI "?" NUM "K", erase_in_line ) \
  CSQ( CSI "L", insert_lines ) \
  CSQ( CSI NUM "L", insert_lines ) \
  CSQ( CSI "M", delete_lines ) \
  CSQ( CSI NUM "M", delete_lines ) \
  CSQ( CSI "P", delete_characters ) \
  CSQ( CSI NUM "P", delete_characters ) \
  CSQ( CSI "S", scroll_page_up ) \
  CSQ( CSI NUM "S", scroll_page_up ) \
  CSQ( CSI "T", scroll_page_down ) \
  CSQ( CSI NUM "T", scroll_page_down ) \
  CSQ( CSI "d", line_position_absolute ) \
  CSQ( CSI NUM "d", line_position_absolute ) \
  CSQ( CSI "f", horizontal_vertical_position ) \
  CSQ( CSI NUM "f", horizontal_vertical_position ) \
  CSQ( CSI "`", cursor_horizontal_absolute /* character position absolute */ ) \
  CSQ( CSI NUM "`", cursor_horizontal_absolute /* character position absolute */ ) \
  CSQ( CSI "m", character_attribute_change ) \
  CSQ( CSI NUM "m", character_attribute_change ) \
  CSQ( CSI NUM "n", device_status_report ) \
  CSQ( CSI "s", save_cursor_position ) \
  CSQ( CSI "u", restore_cursor_position ) \
  CSQ( CSI "?" NUM "h", enable ) \
  CSQ( CSI "?" NUM "l", disable ) \
  CSQ( OSC SNUM ";" TEXT ST, osc_cmd ) \
  CSQ( OSC SNUM ";" TEXT "\7", osc_cmd )

#define CSQ(A,B) int tym_i_csq_ ## B(struct tym_i_pane_internal* pane) __attribute__((weak));
  CSQS
#undef CSQ

struct tym_i_command_sequence tym_i_command_sequence_map[] = {
#define CSQ(A,B) { \
    .sequence=(A), \
    .length=sizeof(A)-1, \
    .callback_name=(#B), \
    .callback=tym_i_csq_ ## B \
  },
CSQS
#undef CSQ
};
const size_t tym_i_command_sequence_map_count = sizeof(tym_i_command_sequence_map)/sizeof(*tym_i_command_sequence_map);

int command_sequence_comparator_ch(char ca, char cb){
  if(ca == cb)
    return 0;
  if((ca < ' ') == (cb < ' '))
    return (ca < cb) ? -1 : 1;
  return ca < ' ' ? 1 : -1;
}

int command_sequence_comparator_str(const char*restrict ia, const char*restrict ib){
  for( ; *ia && *ib; ia++, ib++ ){
    int ret = command_sequence_comparator_ch(*ia, *ib);
    if(ret)
      return ret;
  }
  if(*ia) return 1;
  if(*ib) return -1;
  return 0;
}

int command_sequence_comparator(const void*restrict pa, const void*restrict pb){
  const struct tym_i_command_sequence* a = pa;
  const struct tym_i_command_sequence* b = pb;
  return command_sequence_comparator_str(a->sequence, b->sequence);
}

static void init(void) __attribute__((constructor,used));
static void init(void){
  qsort(tym_i_command_sequence_map, tym_i_command_sequence_map_count, sizeof(*tym_i_command_sequence_map), command_sequence_comparator);
//  for(size_t i=0; i<tym_i_command_sequence_map_count; i++)
//    fprintf(stderr,": %s\n",tym_i_command_sequence_map[i].sequence);
}

enum specialmatch_result {
  SM_NO_MATCH,
  SM_MATCH_SINGLE,
  SM_MATCH_CONTINUE
};

static enum specialmatch_result specialmatch(unsigned char a, unsigned char b){
  switch(a){
    case '\1': return SM_MATCH_SINGLE;
    case '\2': return (b >= '0' && b <= '9') || b == ';' ? SM_MATCH_CONTINUE : SM_NO_MATCH;
    case '\3': return (b >= '0' && b <= '9') ? SM_MATCH_CONTINUE : SM_NO_MATCH;
    case '\4': return b >= ' ' ? SM_MATCH_CONTINUE : SM_NO_MATCH;
    default: return false;
  }
}

void reset_sequence(struct tym_i_sequence_state* sequence){
  sequence->last_special_match_continue = false;
  sequence->seq_opt_max = -1;
  sequence->length = 0;
  sequence->index = 0;
  sequence->integer_count = 0;
  memset(sequence->integer, 0, sizeof(int) * TYM_I_MAX_INT_COUNT);
}

bool seq_parse_update(struct tym_i_pane_internal* pane, unsigned char c){
  unsigned short n = pane->sequence.length;
  unsigned short index = pane->sequence.index;
  if(n >= TYM_I_MAX_SEQ_LEN)
    goto escape_abort;
  if(!pane->sequence.length){
    pane->sequence.seq_opt_min = 0;
    pane->sequence.seq_opt_max = tym_i_command_sequence_map_count-1;
  }
  ssize_t min = pane->sequence.seq_opt_min;
  ssize_t max = pane->sequence.seq_opt_max;
  if( pane->sequence.last_special_match_continue && !specialmatch(tym_i_command_sequence_map[min].sequence[index], c) )
    index++;
  while( max >= min && ( tym_i_command_sequence_map[min].length <= index || (
         tym_i_command_sequence_map[min].length > index
      && command_sequence_comparator_ch(tym_i_command_sequence_map[min].sequence[index], c) < 0
      && !specialmatch(tym_i_command_sequence_map[min].sequence[index], c)
  ))) min++;
  if( max >=min && tym_i_command_sequence_map[min].sequence[index] >= ' ' && command_sequence_comparator_ch(tym_i_command_sequence_map[min].sequence[index], c) > 0)
  while( max >= min && (
        tym_i_command_sequence_map[min].length <= index // Order sensitive!!
     || tym_i_command_sequence_map[min].sequence[index] >= ' '
     || !specialmatch(tym_i_command_sequence_map[min].sequence[index], c)
  )) min++;
  bool esm = false;
  if(max >= min && tym_i_command_sequence_map[min].length > index)
    if(specialmatch(tym_i_command_sequence_map[min].sequence[index], c))
      esm = true;
  while( max >= min && tym_i_command_sequence_map[max].length > index
      && command_sequence_comparator_ch(tym_i_command_sequence_map[max].sequence[index], c) > 0
      && (!esm||!specialmatch(tym_i_command_sequence_map[min].sequence[index], c))
  ) max--;
/*  if(max < min)
    fprintf(stderr,"%*s %c\n",(int)pane->sequence.length,pane->sequence.buffer, c);*/
  if(max < min || index >= tym_i_command_sequence_map[min].length || index >= tym_i_command_sequence_map[max].length)
    goto escape_abort;
  pane->sequence.seq_opt_min = min;
  pane->sequence.seq_opt_max = max;
  if(max >= min && tym_i_command_sequence_map[min].sequence[index] < ' '){
    switch(tym_i_command_sequence_map[min].sequence[index]){
      case '\2': {
        if(!pane->sequence.integer_count)
          pane->sequence.integer_count = 1;
        if(c == ';'){
          if(++pane->sequence.integer_count >= TYM_I_MAX_INT_COUNT)
            goto escape_abort;
        }else if(c >= '0' && c <= '9'){
          pane->sequence.integer[pane->sequence.integer_count-1] *= 10;
          pane->sequence.integer[pane->sequence.integer_count-1] += c - '0';
        }else goto escape_abort;
      } break;
    }
  }
  if(min == max && index+1 == tym_i_command_sequence_map[min].length){
    const struct tym_i_command_sequence* sequence = tym_i_command_sequence_map + min;
    if(sequence->callback){
      if((*sequence->callback)(pane) == -1){
        int err = errno;
        tym_i_debug("%s failed: %s\n", sequence->callback_name, strerror(err));
        if(err == ENOENT)
          goto escape_abort;
      }
    }else{
      tym_i_debug("%s unimplemented\n", sequence->callback_name);
    }
    reset_sequence(&pane->sequence);
    return true;
  }
  pane->sequence.buffer[n] = c;
  pane->sequence.length = n + 1;
  if( specialmatch(tym_i_command_sequence_map[min].sequence[index], c) == SM_MATCH_CONTINUE ){
    pane->sequence.last_special_match_continue = true;
  }else{
    pane->sequence.index = index + 1;
  }
  return true;
escape_abort:;
  if(pane->sequence.buffer[0] == *ESC)
    pane->sequence.buffer[0] = '^';
  tym_i_debug("%.*s%c\n",(int)pane->sequence.length,pane->sequence.buffer,c);
  reset_sequence(&pane->sequence);
  for(unsigned short i=0; i<n; i++)
    tym_i_pane_parse(pane, pane->sequence.buffer[i]);
  tym_i_pane_parse(pane, c);
  return false;
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

static int mysetattr(struct tym_i_pane_internal* pane){
  if(!has_colors())
    return -1;
  int pair = -1;
  attr_t attr = attr2curses(pane->attribute);
  if(COLOR_PAIRS >= 45){
    int fi = 0;
    int bi = 0;
    if(pane->fgcolor.index == 0xFF){
      // TODO
    }else if(pane->fgcolor.index < 20){
      fi = pane->fgcolor.index % 10;
    }
    if(pane->bgcolor.index == 0xFF){
      // TODO
    }else if(pane->bgcolor.index < 20){
      bi = pane->bgcolor.index % 10;
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
  wattr_set(pane->window, attr, pair, 0);
  return 0;
}

void tym_i_pane_parse(struct tym_i_pane_internal* pane, unsigned char c){ // TODO: move everything in here to seq_parse_update
  unsigned y = pane->cursor.y;
  unsigned x = pane->cursor.x;
  unsigned w = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[0].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[0].value.integer;
  unsigned h = pane->coordinates.position[TYM_P_CHARFIELD][1].axis[1].value.integer - pane->coordinates.position[TYM_P_CHARFIELD][0].axis[1].value.integer;
  if(x >= w)
    x = w;
  if(y >= h)
    y = h;
  if(!pane->sequence.length){
    switch(c){
      case '\b': if(x) x -= 1; break;
      case '\r': x = 0; break;
      case '\n': {
        x  = 0;
        y += 1;
      } break;
      case '\x1B': { // *ESC
        seq_parse_update(pane, c);
      }; break;
      default: {
        if(c < ' ')
          break;
        mysetattr(pane);
        mvwaddch(pane->window, y, x, c);
        x += 1;
        if(x >= w){
          x  = 0;
          y += 1;
        }
      } break;
    }
    tym_i_pane_cursor_set_cursor(pane,x,y);
  }else{
    seq_parse_update(pane, c);
  }
  tym_i_pane_update_cursor(pane);
}

