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
#include <internal/backend.h>
#include <internal/parser.h>

/** \file */

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

/*
  CSQ( ESC "P",  ) \
  CSQ( ESC "V",  ) \
  CSQ( ESC "W",  ) \
  CSQ( ESC "X",  ) \
*/

/**
 * This is a list of all escape sequences and the corresponding callback function.
 * There is a limitation though: don't specify NUM and an actuall number in different
 * escape sequences with the same start, the parser isn't designed to handle that.
 * Same for SNUM, TEXT and similar macros.
 */
#define CSQS \
  CSQ( RIS, reset ) \
  CSQ( ESC "#3", double_height_line_top_half ) \
  CSQ( ESC "#4", double_height_line_bottom_half ) \
  CSQ( ESC "#5", single_width_line ) \
  CSQ( ESC "#6", double_width_line ) \
  CSQ( ESC "#8", screen_alignment_test ) \
  CSQ( ESC "(" C, designate_g0_character_set ) \
  CSQ( ESC ")" C, designate_g1_character_set ) \
  CSQ( ESC "*" C, designate_g2_character_set ) \
  CSQ( ESC "+" C, designate_g3_character_set ) \
  CSQ( ESC "%@", select_default_character_set ) \
  CSQ( ESC "%G", select_utf8_character_set ) \
  CSQ( ESC "n", invoke_charset_G1_as_GL_SL2 ) \
  CSQ( ESC "o", invoke_charset_G2_as_GL_SL3 ) \
  CSQ( ESC "|", invoke_charset_G2_as_GR_LS3R ) \
  CSQ( ESC "}", invoke_charset_G3_as_GR_LS2R ) \
  CSQ( ESC "~", invoke_charset_G0_as_GR_LS1R ) \
  CSQ( ESC "7", save_cursor_position ) \
  CSQ( ESC "8", restore_cursor_position ) \
  CSQ( ESC "=", application_keypad /* DECPAM */ ) \
  CSQ( ESC ">", normal_keypad /* DECPNM */ ) \
  CSQ( ESC "F", cursor_to_bottom_left ) \
  CSQ( ESC "l", memory_lock ) \
  CSQ( ESC "m", memory_unlock ) \
  CSQ( ESC "D", cursor_down /* index (NEL) */ ) \
  CSQ( ESC "E", cursor_next_line /* next line */ ) \
  CSQ( ESC "H", tab_set ) \
  CSQ( ESC "M", reverse_index ) \
  CSQ( ESC "N" C, g2_character_set_for_character ) \
  CSQ( ESC "O" C, g3_character_set_for_character ) \
  CSQ( ESC "Z", send_device_attributes_primary ) \
  CSQ( CSI "r", set_scrolling_region ) \
  CSQ( CSI NUM "r", set_scrolling_region ) \
  CSQ( CSI "!p", soft_reset ) \
  CSQ( CSI NUM "h", set_mode ) \
  CSQ( CSI NUM "l", reset_mode ) \
  CSQ( CSI "c", send_device_attributes_primary ) \
  CSQ( CSI NUM "c", send_device_attributes_primary ) \
  CSQ( CSI "e", vertical_position_relative /* VPR aka. line position forward */ ) \
  CSQ( CSI NUM "e", vertical_position_relative ) \
  CSQ( CSI "a", character_position_relative /* VPR aka. character position forward */ ) \
  CSQ( CSI NUM "a", character_position_relative ) \
  CSQ( CSI "g", tab_clear /* TBC */ ) \
  CSQ( CSI NUM "g", tab_clear ) \
  CSQ( CSI "k", vertical_position_backwards /* VPB */ ) \
  CSQ( CSI NUM "k", vertical_position_backwards ) \
  CSQ( CSI "@", insert_character /* ICH */ ) \
  CSQ( CSI NUM "@", insert_character ) \
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
  CSQ( CSI "I", cursor_forward_tabulation ) \
  CSQ( CSI NUM "I", cursor_forward_tabulation ) \
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
  CSQ( CSI "S", scroll_up ) \
  CSQ( CSI NUM "S", scroll_up ) \
  CSQ( CSI "T", scroll_down ) \
  CSQ( CSI NUM "T", scroll_down ) \
  CSQ( CSI "X", erase_characters ) \
  CSQ( CSI NUM "X", erase_characters ) \
  CSQ( CSI "d", line_position_absolute ) \
  CSQ( CSI NUM "d", line_position_absolute ) \
  CSQ( CSI "f", cursor_position ) \
  CSQ( CSI NUM "f", cursor_position ) \
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

#define CSQ(A,B) { \
    .sequence=(A), \
    .length=sizeof(A)-1, \
    .callback_name=(#B), \
    .callback=tym_i_csq_ ## B \
  },
/**
 * This is an array of all escape sequences. This will be sorted using the 
 * using the command_sequence_comparator in the init function in this file.
 */
static struct tym_i_command_sequence tym_i_command_sequence_map[] = {
CSQS
};
#undef CSQ
static const size_t tym_i_command_sequence_map_count = sizeof(tym_i_command_sequence_map)/sizeof(*tym_i_command_sequence_map);

/** A comperator for characters in the escape sequence template. */
static int command_sequence_comparator_ch(char ca, char cb){
  if(ca == cb)
    return 0;
  if((ca < ' ') == (cb < ' '))
    return (ca < cb) ? -1 : 1;
  return ca < ' ' ? 1 : -1;
}

/** A comperator for escape sequences. */
static int command_sequence_comparator_str(const char*restrict ia, const char*restrict ib){
  for( ; *ia && *ib; ia++, ib++ ){
    int ret = command_sequence_comparator_ch(*ia, *ib);
    if(ret)
      return ret;
  }
  if(*ia) return 1;
  if(*ib) return -1;
  return 0;
}

/** same as command_sequence_comparator_str, wrapper for qsort. */
static int command_sequence_comparator(const void*restrict pa, const void*restrict pb){
  const struct tym_i_command_sequence* a = pa;
  const struct tym_i_command_sequence* b = pb;
  return command_sequence_comparator_str(a->sequence, b->sequence);
}

static void init(void) __attribute__((constructor,used));
static void init(void){
  qsort(tym_i_command_sequence_map, tym_i_command_sequence_map_count, sizeof(*tym_i_command_sequence_map), command_sequence_comparator);
  bool notice_printed = false;
  for(size_t i=0; i<tym_i_command_sequence_map_count; i++){
    if(tym_i_command_sequence_map[i].callback)
      continue;
    if(!notice_printed){
      tym_i_debug("The following escape sequence actions aren't implemented yet:\n");
      notice_printed = true;
    }
    tym_i_debug(" - %s\n",tym_i_command_sequence_map[i].callback_name);
  }
}

enum specialmatch_result {
  SM_NO_MATCH,
  SM_MATCH_SINGLE,
  SM_MATCH_CONTINUE
};

/** Handle special constants in the escape sequence template which could stand for multiple different characters. */
static enum specialmatch_result specialmatch(unsigned char a, unsigned char b){
  switch(a){
    case '\1' /* C */: return SM_MATCH_SINGLE;
    case '\2' /* SNUM */: return (b >= '0' && b <= '9') || b == ';' ? SM_MATCH_CONTINUE : SM_NO_MATCH;
    case '\3' /* NUM  */: return (b >= '0' && b <= '9') ? SM_MATCH_CONTINUE : SM_NO_MATCH;
    case '\4' /* TEXT */: return b >= ' ' ? SM_MATCH_CONTINUE : SM_NO_MATCH;
    default: return SM_NO_MATCH;
  }
}

/** Reset the parser state for the current sequence */
static void reset_sequence(struct tym_i_sequence_state* sequence){
  sequence->last_special_match_continue = false;
  sequence->seq_opt_min = 0;
  sequence->seq_opt_max = -1;
  sequence->length = 0;
  sequence->index = 0;
  sequence->integer_count = 0;
  memset(sequence->integer, 0, sizeof(int) * TYM_I_MAX_INT_COUNT);
}

/** Check if a character is a control character and interpret it. */
static bool control_character(struct tym_i_pane_internal* pane, unsigned char c){
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned y = screen->cursor.y;
  unsigned x = screen->cursor.x;
  unsigned w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  if(x >= w)
    x = w;
  if(y >= h)
    y = h;
  switch(c){
    case '\b': if(x) x -= 1; break;
    case '\r': x = 0; break;
    case '\t': x = x / 8 * 8 + 8; break;
    case '\v': y += 1; break;
    case '\n': y += 1; break; // TODO: implement automatic new line
    case 0x0E /*SO*/: tym_i_invoke_charset(pane, TYM_I_CHARSET_SELECTION_GL_G1 | TYM_I_CHARSET_SELECTION_GR_G1); break;
    case 0x0F /*SI*/: tym_i_invoke_charset(pane, TYM_I_CHARSET_SELECTION_GL_G0 | TYM_I_CHARSET_SELECTION_GR_G0); break;
  }
  if(c >= ' ')
    return false;
  tym_i_debug("Control character: x%02X\n",(int)c);
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_ABSOLUTE, x,
    TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY, TYM_I_SCP_PM_ABSOLUTE, y,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE, false
  );
  return true;
}

/** A character for representing invalid characters */
static const struct tym_i_character UTF8_INVALID_SYMBOL = {
  .data = {
    .utf8 = {
      .data = "�", // ef bf bd
      .count = 3,
    }
  }
};

/** Print a character after converting it to utf-8 */
static void print_character(struct tym_i_pane_internal* pane, const struct tym_i_character character){
  if(tym_i_character_is_utf8(character) && !character.data.utf8.count)
    return;
  struct tym_i_pane_screen_state* screen = &pane->screen[pane->current_screen];
  unsigned y = screen->cursor.y;
  unsigned x = screen->cursor.x;
  unsigned w = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_HORIZONTAL);
  unsigned h = TYM_RECT_SIZE(pane->absolute_position, CHARFIELD, TYM_AXIS_VERTICAL);
  if(x >= w)
    x = w;
  if(y >= h)
    y = h;
  const char* sequence = 0;
  if(tym_i_character_is_utf8(character)){
    sequence = (char*)character.data.utf8.data;
  }else{
    bool codepage = !!(character.data.byte & 0x80);
    uint8_t index = character.data.byte & 0x7F;
    uint8_t gl = character.charset_selection;
    uint8_t gr = character.charset_selection >> 8;
    uint8_t g = codepage ? gr : gl;
    if(g >= TYM_I_G_CHARSET_COUNT || index < ' '){
      sequence = (char*)UTF8_INVALID_SYMBOL.data.utf8.data;
    }else{
      uint8_t charset = character.charset_g[g];
      if(charset >= TYM_I_CHARSET_COUNT){
        sequence = (char*)UTF8_INVALID_SYMBOL.data.utf8.data;
      }else{
        sequence = (char*)tym_i_translation_table[charset].table[index-' '];
      }
    }
  }
  if(x >= w){
    x = 0;
    if(!screen->wraparound_mode_off)
      y += 1;
  }
  if(sequence)
    tym_i_backend->pane_set_character(pane, (struct tym_i_cell_position){.x=x,.y=y}, screen->character_format, strlen(sequence), sequence, screen->insert_mode);
  x += 1;
  tym_i_pane_set_cursor_position( pane,
    TYM_I_SCP_PM_ABSOLUTE, x,
    TYM_I_SCP_SMM_SCROLL_FORWARD_ONLY, TYM_I_SCP_PM_ABSOLUTE, y,
    TYM_I_SCP_SCROLLING_REGION_UNCROSSABLE, true
  );
}

/** Add a byte to a utf-8 character buffer and print complete ones. */
static bool print_character_update(struct tym_i_pane_internal* pane, char c){
  if(tym_i_character_is_utf8(pane->character)){
    enum tym_i_utf8_character_state_push_result result = tym_i_utf8_character_state_push(&pane->character.data.utf8, c);
    if(result & TYM_I_UCS_INVALID_ABORT_FLAG){
      print_character(pane, UTF8_INVALID_SYMBOL);
      memset(&pane->character.data.utf8, 0, sizeof(pane->character.data.utf8));
      return false;
    }else if(result == TYM_I_UCS_DONE){
      print_character(pane, pane->character);
      memset(&pane->character.data.utf8, 0, sizeof(pane->character.data.utf8));
    }
  }else{
    pane->character.data.byte = c;
    print_character(pane, pane->character);
  }
  return true;
}

/** Write the sequences and parameters to the debug output */
void tym_i_debug_sequence_params(const struct tym_i_command_sequence* command, const struct tym_i_sequence_state* state){
  tym_i_debug("%s", command->callback_name);
  if(state->integer_count){
    tym_i_debug(" i(");
    for(size_t i=0; i<state->integer_count; i++){
      int x = state->integer[i];
      tym_i_debug(i ? ", %d" : "%d", x);
    }
    tym_i_debug(")");
  }
}

/**
 * This is the parser/interpreter. The sequences are sorted, so this parser just checks
 * if a character is larger or smaller than the corresponding one of the first/past possible
 * sequence, and adjusts those boundaries until only one is left and matches.
 * If no sequence matches, the first character is sent directly and the remaining ones
 * are parsed using this function again.
 */
void tym_i_pane_parse(struct tym_i_pane_internal* pane, unsigned char c){
//  tym_i_debug("tym_i_pane_parse %.2X\n", c);
  if(tym_i_character_is_utf8(pane->character))
    if( pane->character.data.utf8.count )
      if(print_character_update(pane, c))
        return;
  unsigned short n = pane->sequence.length;
  unsigned short index = pane->sequence.index;
  if(n >= TYM_I_MAX_SEQ_LEN)
    goto escape_abort;
  if(!pane->sequence.length){
    pane->sequence.seq_opt_min = 0;
    pane->sequence.seq_opt_max = tym_i_command_sequence_map_count-1;
    
    // Optional optimisation: The fist character is currently always \x1B, checking every single
    // entry in that case would be unnecessary and really inefficent. Let's check that right away
    // in a way that won't break if that changes some day.
    if( command_sequence_comparator_ch(tym_i_command_sequence_map[0].sequence[index], c) > 0
     || command_sequence_comparator_ch(tym_i_command_sequence_map[tym_i_command_sequence_map_count-1].sequence[index], c) < 0
    ) pane->sequence.seq_opt_max = -1;
    // end optimisation
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
  if(max < min || index >= tym_i_command_sequence_map[min].length || index >= tym_i_command_sequence_map[max].length){
    if(control_character(pane, c)){
      return;
    }else{
      goto escape_abort;
    }
  }
  pane->sequence.seq_opt_min = min;
  pane->sequence.seq_opt_max = max;
  if(max >= min && tym_i_command_sequence_map[min].sequence[index] < ' '){
    switch(tym_i_command_sequence_map[min].sequence[index]){
      case '\1': {
        if(++pane->sequence.integer_count >= TYM_I_MAX_INT_COUNT)
          goto escape_abort;
        pane->sequence.integer[pane->sequence.integer_count-1] = c;
      } break;
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
    const struct tym_i_command_sequence* command = tym_i_command_sequence_map + min;
    if(command->callback){
      if(command->callback(pane) == -1){
        int err = errno;
        tym_i_debug("- ");
        tym_i_debug_sequence_params(command, &pane->sequence);
        tym_i_debug(": %d %s\n", err, strerror(err));
        if(err == ENOENT)
          goto escape_abort;
      }else{
        tym_i_pane_update_cursor(pane);
        tym_i_debug("+ ");
        tym_i_debug_sequence_params(command, &pane->sequence);
        tym_i_debug("\n");
      }
    }else{
      tym_i_debug("? ");
      tym_i_debug_sequence_params(command, &pane->sequence);
      tym_i_debug("\n");
    }
    reset_sequence(&pane->sequence);
    return;
  }
  pane->sequence.buffer[n] = c;
  pane->sequence.length = n + 1;
  if( specialmatch(tym_i_command_sequence_map[min].sequence[index], c) == SM_MATCH_CONTINUE ){
    pane->sequence.last_special_match_continue = true;
  }else{
    pane->sequence.index = index + 1;
  }
  return;
escape_abort:;
  char fc = pane->sequence.length ? pane->sequence.buffer[0] : c;
  switch(fc){
    case '\x1B': print_character_update(pane, '^'); break;
    default: print_character_update(pane, fc); break;
  }
  if(pane->sequence.length){
    tym_i_debug("%.*s%c %zd %zd\n",(int)pane->sequence.length,pane->sequence.buffer,c, pane->sequence.seq_opt_min, pane->sequence.seq_opt_max);
    reset_sequence(&pane->sequence);
    for(unsigned short i=1; i<n; i++)
      tym_i_pane_parse(pane, pane->sequence.buffer[i]);
    tym_i_pane_parse(pane, c);
  }
  tym_i_pane_update_cursor(pane);
  return;
}

