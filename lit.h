#pragma once
#define EOLN	0xff
#define _TRUE	0xff
#define _FALSE	0

#define CONTROLC	3
#define BELL	7
#define CR	13
#define TAB	9
#define LF	10
#define ESC	0x1b
#define SPACE	' '
#define RUBOUT	0x7f
#define ESCSTR  "\x1b"


/*****     LIT UDI     *****/
#define E_BASE  128
#define E_MEM	(E_BASE + 0x2)
//#define E_FNEXIST	(E_BASE + 0x21)
#define E_FEXIST	(E_BASE + 0x20)
#define E_NOT_SUPPORTED	(E_BASE + 0x23)
#define E_STRING_BUFFER (E_BASE + 0x81)
#define E_OK     0



/* seek modes */
#define relative_back_mode	1
#define abs_mode	2
#define relative_forward_mode	3
#define end_minus_mode	4

/*****     LIT  PUBLIC    *****/

#define keep_file	0
#define lose_file	1
#define lost_file	2
#define view_only	3
#define view_only_wk1	4
#define view_only_wk2	5

#define upper_window	0
#define lower_window	1

#define window_minimum	512

#define RVID	"\xc7"      /* string version */
#define rvid    0xc7

#define ed_taga	5
#define ed_tagb	6
#define ed_tagi	7
#define ed_tagw	8

#define num_tag	8

#define filename_len	60
#define filename_len_plus_1	61
#define string_len	60
#define string_len_plus_1	61
#define string_len_plus_2	62

#define rmx_string	16


#define s_input_file "oa.input_name"
#define s_output_file "oa.output_name"

#define command_mode	0
#define insert_mode	1
#define xchange_mode	2

#define t_blank	0
#define t_identifier	1
#define t_other	2

#define ci_file	0
#define co_file	1
#define mac_file	2
#define in_file	3
#define out_file	4
#define util_file	5
#define block_file	6
#define work_file	6 /* only for error messages. */


/*****     LIT CMNDS     *****/

#define ubuf_size	100
#define not_hex	99


/*****     LIT CONF     *****/

#define UNKNOWN	0
#define SIV	1
#define SIII	2
#define SIIIE	3
#define SIIIEO	4
#define SIIIET	5
#define ANSI	6
#define VT100	7
#define IBMPC	8


/*! The order is significant !!!!!. */

#define first_code	0xe8
#define up_code	0xe8
#define down_code	0xe9
#define left_code	0xea
#define right_code	0xeb
#define home_code	0xec
#define delete_code	0xed
#define esc_code	0xee
#define delete_line_code	0xef
#define delete_left_code	0xf0
#define delete_right_code	0xf1
#define undo_code	0xf2
#define rubout_code	0xf3
#define macro_exec_code	0xf4
#define s_var_code	0xf5
#define n_var_code	0xf6
#define hexin_code	0xf7
#define ignore_code	0xf8
#define last_code	0xf8
#define illegal_code	0xff

#define col_first	0
#define row_first	1
#define ANSI_RC	2

#define up_out_code	0
#define down_out_code	1
#define left_out_code	2
#define right_out_code	3
#define home_out_code	4
#define back_out_code	5
#define erase_screen_out_code	6
#define erase_entire_screen_out_code	7
#define erase_line_out_code	8
#define erase_entire_line_out_code	9
#define reverse_scroll_out_code	10
#define goto_out_code	11
#define offset_index	12
#define delete_out_code	13
#define reverse_video_code	14
#define normal_video_code	15
#define blank_out_code	16


/*****   LIT AEDDUM   *****/

#define memory_leng	48000

/* based variables */
/*****     PUB.PLM    *****/
#define low_s_byte (*oa.low_s)
#define low_e_byte (*oa.low_e)
#define high_s_byte (*oa.high_s)
#define high_e_byte (*oa.high_e)

/*****     UTIL.PLM     *****/
#define scan_byte (*scan_p)

// macros to synthesize 2 and 4 byte char values supported in PLM
#define CHAR2(c1, c2)   (((c1) << 8) + (c2))
#define CHAR4(c1, c2, c3, c4)   (((c1) << 24) + ((c2) << 16) + ((c3) << 8) + (c4))