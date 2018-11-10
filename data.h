#pragma once
#include <setjmp.h>
#include "type.h"
#include "lit.h"

/*****     VERV.PLM    *****/

extern byte copy_right_message[];
extern byte copy_right[];
extern byte title[];
extern byte version[];

/*****     AEDIT.PLM     *****/

extern pointer prompts[];
extern jmp_buf aedit_entry;

/*****     PUB.PLM    *****/

extern oa_struc oa;
extern oa_struc ob;

extern byte delimiters[];
/*! this array tells whether or not the given character is a delimiter
       or not 1->delimiter, 0-> non delimiter  */

extern byte w_dirty;
/*! when any edit is done on the file that the upper window is pointing
 to, this byte will be set, so when we go to display the upper window,
 we can replot it */


extern pointer cursor;


extern byte row;
extern byte col;
extern byte virtual_col;

extern byte wrapping_column;
extern pointer have[];
extern pointer saved_from;
extern byte saved_bytes[];
extern byte line_size[];

extern byte first_text_line;
extern byte last_text_line;
extern byte message_line;
extern byte prompt_line;

extern bool window_present;
extern byte window_num;

/*! The following structure is used to store the values for the
       alternate window on the screen [if present] */
extern window_t window; 
extern byte force_column;

extern byte command;
extern byte last_cmd;
extern byte last_main_cmd;
extern word count;
extern byte infinite;

extern byte auto_indent;
extern byte backup_files;
extern byte blank_for_tab;
extern byte crlf_insert;
extern byte find_uplow;
extern byte show_find;
extern byte token_find;
extern byte center_line;
extern byte dont_stop_macro_output;
extern byte radix;
extern boolean highbit_flag;

extern byte last_mode;
extern byte cur_mode;
extern byte lstfnd;
extern byte cursor_type;

extern  byte tab_to[255];
/*! TAB_TO CONTAINS THE NUMBER OF SPACES THAT SHOULD BE OUTPUT
    WHEN A TAB IS FOUND IN THE CORRESPONDING COLUMN 
    DEFAULT IS TAB EVERY 4 COLUMNS */

extern byte ltype[];
/*! LTYPE CLASSIFIES CHARACTERS AS BLANK, IDENTIFIER AND OTHER. EXPECTS
    HIGH BIT TO BE MASKED OFF [LF IS 3] */

extern byte cc_flag;
extern byte have_control_c;
extern byte quit_state;

extern  byte co_buffer[255];

extern  byte input_buffer[string_len_plus_1 + 1];

extern byte tmp_str[81];

extern byte in_macro_def;
extern byte not_in_macro;

extern byte in_other;
extern byte w_in_other;

extern byte macro_exec_level;
extern byte macro_suppress;

extern byte minus_type;

extern word excep;

extern byte hex_digits[];

extern word memory_size;

extern logical isa_text_line;

extern logical in_block_mode;

extern logical batch_mode;
extern logical submit_mode;

extern byte left_margin;
extern byte right_margin;
extern byte indent_margin;

extern byte ci_name[];
extern byte co_name[];
extern byte work_name[];
extern file_t files[7];
#define input_conn  files[in_file].conn         /* INPUT_CONN IS USED BY OTHER COMMAND TO SWITCH CONNETION NUMBERS    */

extern byte file_num;

extern byte null_str;

extern byte no_such_macro[];

extern boolean force_writing;

extern boolean do_type_ahead;

extern word delay_after_each_char;

extern byte invalid_null_name[];

/*****     CCTRAP.PLM     *****/

extern pointer excep_handler_p; /* the type doesn't matter, only the address is taken */

/*****     BLOCK.PLM     *****/

extern byte block_buffer[];
extern word in_block_buffer;
extern boolean first_at_sign_is_here;


/*****     CALC.PLM     *****/

extern byte s_write_file[];
extern byte s_get_file[];
extern byte s_put_file[];
extern byte s_macro_file[];
extern byte s_block_file[];
extern byte s_macro_name[];

extern dword n_var[];

extern byte invalid_name[];


/*****     CMNDS.PLM     *****/

extern byte last_move_command;
extern logical hex_command;
extern logical hex_error;
extern byte hex_length;
extern byte invalid_hex[];
extern byte ubuf[];
extern byte ubuf_left;
extern byte ubuf_used;
extern byte u_is_line;

extern byte prompt_number;


/*****     COMAND.PLM     *****/

extern boolean pause;
extern boolean strip_parity;
extern boolean in_input_line;


/*****     CONF.PLM     *****/

extern byte config;
extern byte wrapper;
extern byte print_as[];
/*! PRINT_AS THE OUTPUT TRANSLATION TABLE - EXPECTS HIGH BIT MASKED OFF
    THIS TABLE IS FOR SERIES IV WHICH CAN DISPLAY MOST FUNNY CHARACTERS
    START WILL PATCH THE TABLE FOR SERIES III's WHICH GENERALLY CANNOT
    DISPLAY THE CONTROL CHARACTERS. */

extern  word input_code_names[17];
extern  code_t input_codes[17];

extern byte first_coordinate;
extern byte visible_attributes;
extern byte character_attributes;

extern  word output_code_names[17];
extern  code_t output_codes[17];    // replace with output_code[17][5] ?
extern  word delay_times[17];
extern boolean dos_system;
extern  byte exit_config_list[41];


/*****     CONSOL.PLM     *****/
extern error_struct_t error_status;
extern byte current_message[];
extern byte next_message[];
extern byte last_message[];
extern byte current_prompt[];
extern byte suppressed_prompt[];
extern boolean bell_on;


/*****    FIND.PLM    *****/

extern byte target[];
extern byte replacement[];
extern dword cnt_fnd;
extern dword cnt_rep;



/*****     IO.PLM     *****/
extern boolean input_expected_flag;
extern byte ci_buff[];
extern byte cur_ci;
extern byte valid_ci;
extern byte cc_code_buf[];
extern boolean poll_mode;
/*****     MACROF.PLM     *****/
extern word macro_buf_size;
extern logical check_for_run_keys;
extern byte macro_fail;
extern byte macro_error_string[];
extern dword cnt_exe[];
extern boolean go_flag[];
/*****     SCREEN.PLM     *****/
extern  byte pad[2];
extern byte msecs_per_pad_char;
/*****     SETCMD.PLM     *****/
/*****     START.PLM     *****/
extern word block_size;
/*! Size of blocks in work-files. */
extern  byte debug_option_list[30];
/*****     TAGS.PLM     *****/
/*****    TMPMAN.PLM    *****/
extern word max_tmpman_mem;
/*****     UTIL.PLM     *****/
extern byte *scan_p;
/*****     VIEW.PLM     *****/
extern byte saved_line;
extern byte psaved_line;
/*****     wordS.PLM     *****/
/*****   AEDDUM.PLM   *****/
/*****   XNXSYS.PLM   *****/
/*!****   INFACE   *****/
/*!****   TRMCAP   *****/


