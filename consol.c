// #SMALL
// #title('CONSOL               CONSOLE INTERFACE ROUTINES')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

/*
    SCREEN CONTAINS THE FULL SCREEN STUFF. THESE ROUTINES JUST
    READ FROM AND WRITE TO THE CONSOLE.
*/
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "oscompat.h"
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"



byte current_message[81] = { 0 }; /* CONTENTS OF MESSAGE LINE */
byte last_message[81] = { 0 };
byte next_message[81] = { "\x6" " -!!- " }; /* NEXT MESSAGE LINE*/

byte current_prompt[81] = { 0 }; /* CONTENTS OF PROMPT LINE*/
byte suppressed_prompt[81] = { 0 };

byte null_line[80] = { "\x4f"
    "                                        "
    "                                      ?" };



#define clean_message	0
#define count_message	1
#define dirty_message	2

byte message_is;                  /* USED TO SPEED UP CLEAR_MESSAGE */
byte w_message_is;                  /* USED TO SPEED UP CLEAR_MESSAGE */





boolean bell_on = _TRUE;


void AeditBeep() {
    /* USELESS NOISEMAKER */


    boolean save_force_writing;

    if (bell_on) {
        save_force_writing = force_writing;
        force_writing = _TRUE;
        Print_string("\x1\a");     /* 7 - bell */
        force_writing = save_force_writing;
    }

} /* beep */




/*    ASSIGN_TO_MESSAGE_IS & CUR_MESSAGE_IS - Utility routines to
    access the message_is and w_message_is parameters
*/

static void Assign_to_message_is(byte newval) {

    if (macro_exec_level == 0 || force_writing) {
        if (!window_present || window_num == lower_window)
            message_is = newval;
        else
            w_message_is = newval;
    }
} /* assign_to_message_is */



static byte Cur_message_is() {
    if (!window_present || window_num == lower_window)
        return message_is;
    else
        return w_message_is;
} /* cur_message_is */






/*
    PRINTABLE                MAKE A CHARACTER PRINTABLE USING PRINT_AS.
*/

byte Printable(byte ch) {

    if (ch >= 0x80) {
        if (highbit_flag)
            return ch;
        else
            return '?';
    }
    return print_as[ch];
} /* printable */



/*
    TEXT_CO                    WRITE ONE BYTE TO TEXT AREA. MAKE CHARACTER
                            PRINTABLE AND RESTORE ROW AND COLUMN. USED TO
                            ADD AND REMOVE THE @ CHARACTER FOR BLOCK.
*/

byte t_str[2] = { 1, 0 };

void Text_co(byte ch) {

    t_str[1] = Printable(ch);
    Print_string(t_str);
    if (++col == 80) {
        if (!wrapper) {
            col = 79;
        }
        else {
            col = 0;
            row++;
            Put_goto(79, row - 1);
        }
    }
    else {
        Put_left();
    }
} /* text_co */




/*
    PRINT_LINE                OUTPUT A STRING. UPDATE COL.
*/
void Print_line(pointer line) {

    byte current_row;
    pointer text;
    boolean no_region;


    current_row = row;
    if (line_size[row] > *line && output_codes[erase_line_out_code].code[0] == 0
        && output_codes[erase_entire_line_out_code].code[0] != 0) {
        Put_erase_entire_line();
        line_size[row] = 0;
    }
    text = line + 1;

    /* if we have 4 or more spaces and if we have absolute cursor addressing,
        and if the previous line length was 0
    */

    no_region = (row == last_text_line && (config == VT100 || config == ANSI));
    if (no_region)
        Reset_scroll_region();

    if (*(dword *)text == CHAR4(' ', ' ', ' ', ' ')
        && line_size[row] == 0 && output_codes[goto_out_code].code[0] != 0) {

        word tmp;    /* blanks counter */

        tmp = skipb(text, ' ', *line);        /* find number of blanks */
        if (tmp == 0xffff)
            tmp = *line;
        text = line + tmp;                    /* adjust text line length */
        text[0] = *line - tmp;
        Put_goto((byte)tmp, row);            /* adjust cursor position */
        Print_string(text);        /* print the rest of the line */
        col = *line;
    }
    else {
        Print_string(line);
        col = *line;
        if (line_size[row] > *line)
            Put_erase_line();
    }

    /*    CAUTION: PUT ERASE LINE MAY HAVE TO WRITE BLANKS TO CLEAR THE LINE */

    line_size[current_row] = *line;

    if (no_region)
        Put_scroll_region(first_text_line, last_text_line);

    Adjust_for_column_80();

} /* print_line */




/*
    ADD_TO_MESSAGE                    ADD A STRING TO NEXT_MESSAGE.
*/

static void Add_to_message(pointer addstr) {

    byte max_message_len;
    byte i;

    i = addstr[0];                    /* LENGTH OF ADDITIONAL STRING    */
    max_message_len = 80;
    if (i + next_message[0] > max_message_len)
        i = max_message_len - next_message[0];

    /*    ADD THE NEW TEXT    */
    memcpy(&next_message[next_message[0]] + 1, addstr + 1, i);
    next_message[0] = next_message[0] + i;
} /* add_to_message */





static void Print_message_and_stay(pointer line) {
    byte i;
    byte msg[string_len_plus_1];

    if (macro_exec_level != 0 && !force_writing)
        return;

    /* CHOP NEXT_MESSAGE BACK TO ' ---- ' WHICH IS ALWAYS PRESENT */
    next_message[0] = 6;

    if (in_other)
        Add_to_message("\x6" "Other ");
    if (oa.file_disposition == view_only)
        Add_to_message("\x5" "View ");
    if (oa.file_disposition == lose_file || oa.file_disposition == lost_file)
        Add_to_message("\x8" "Forward ");
    if (in_macro_def)
        Add_to_message("\x6" "Macro ");


    /*    ADD THE ACTUAL VOLITILE PART OF MESSAGE TO STRING    */
    /* create a local copy to avoid changing read only strings*/
    memcpy(msg, line, *line + 1);
    for (i = 1; i <= msg[0]; i++) {
        msg[i] = Printable(msg[i]);
    }
    Add_to_message(msg);

    /*    USE THE NORMAL '!' CONVERTION IF LINE IS TOO LONG    */
    if (next_message[0] >= 80)
        next_message[80] = '!';

    if (batch_mode) {
        if (line[0] > 5) { /* don't print COUNT while in batch mode */
            Co_flush();
            Print_unconditionally_p(next_message);
            Print_unconditionally_crlf();
        }
    }
    else {
        Rebuild(message_line, next_message, current_message);
        Move_name(msg, last_message);
    }
    Move_name(next_message, current_message);
    Assign_to_message_is(dirty_message);    /* MESSAGE HAS A REAL MESSAGE */
} /* print_message_and_stay */




/* PRINTS THE COUNT THAT IS IN INPUT_BUFFER ON THE MESSAGE LINE. */

void Print_count_message() {
    Print_message_and_stay(input_buffer);
    Assign_to_message_is(count_message);
    /* kill count_message after scrolling */
    last_message[0] = 0;

} /* print_count_message */




/*   PRINT MESSAGE LINE AND RESTORE CURSOR */
void Print_message(pointer line) {
    byte old_row, old_col;

    old_col = col;
    old_row = row;
    if (macro_exec_level == 0 || force_writing) {
        Print_message_and_stay(line);
        Put_goto(old_col, old_row);
    }
} /* print_message */


error_struct_t error_status = { _FALSE, _FALSE, _FALSE };      // see type.h for description


void Error(pointer msg_p) {

    byte delay;
    byte i;
    boolean save_force_writing;

    save_force_writing = force_writing;
    force_writing = _TRUE;
    if (batch_mode) {
        Print_message(msg_p);
        error_status.key_was_space = _TRUE; /*always continue */
    }
    else if (error_status.in_invocation) {
        /* non-fatal errors on invocation may appear before the
           configuration is known, so use the normal UDI dq$write. */
        Print_unconditionally_p(msg_p);
        Print_unconditionally_crlf();
        Print_unconditionally("hit space to continue");
        Print_unconditionally_crlf();
        Print_unconditionally("\a");
        error_status.key_was_space = ((Ci() & 0x7f) == ' ');
        /* LOOK FOR a keystroke FOR PERMISSION */
    }
    else {
        if (macro_suppress)  /* make row and col valid */
            Put_home();
        Print_message(msg_p);
        AeditBeep();
        Co_flush();
        if (macro_exec_level != 0)
            delay = 75;
        else
            delay = 30;
        // TODO: rewite for high performance PC
        for (i = 1; i <= delay; i++) {     /* every cycle lasts 10 miliseconds @ 5MHz*/
            ms_sleep(10);
            Check_for_keys();
        }
        if (error_status.from_macro_file) {
            error_status.key_was_space = (Hit_space() == ' ');
        }
    }
    force_writing = save_force_writing;

} /* error */





/*************************************************************
    Prints errors which are detected before screen setup.
*************************************************************/
void Early_error(pointer msg) {

    Print_unconditionally_crlf();
    Build_banner(); /* builds the banner in tmp_str */
    Print_unconditionally_p(tmp_str);
    Print_unconditionally_crlf();
    Print_unconditionally("*** ERROR: ");
    Print_unconditionally_p(msg);
    Print_unconditionally_crlf();
    Print_unconditionally_crlf();
    Close_ioc();
    exit(1);
} /* early_error */




/*
    ILLEGAL_COMMAND                        JUST PRINT THE MESSAGE ILLEGAL COMMAND.
*/

void Illegal_command() {
    Error("\xf" "illegal command");
} /* illegal_command */




/*
    KILL_MESSAGE                        BLANK OUT THE MESSAGE LINE.
*/

void Kill_message() {

    Print_message("");
    Assign_to_message_is(clean_message);
} /* kill_message */




/*
    CLEAR_MESSAGE                        CLEARS MESSAGE LINE. WIPES OUT MESSAGE
                                        LINE IF IT IS DIRTY.
*/

void Clear_message() {

    if (Cur_message_is() == dirty_message)
        Kill_message();

} /* clear_message */




/*
    CLEAR_COUNT_MESSAGE                    CLEARS MESSAGE LINE IF IT CONTAINS A
                                        COUNT BUT OTHERWISE LEAVES IT ALONE.
*/

void Clear_count_message() {

    if (Cur_message_is() == count_message)
        Kill_message();

} /* clear_count_message */




/*
    GONE_PROMPT                    SET PROMPT AND MESSAGE LINES TO CLEAR
*/

void Gone_prompt() {

    Assign_to_message_is(count_message);
    /* NULL MESSAGE IS SAME AS REAL MESSAGE.    */
    current_message[0] = 0; /* FLAG AS CLEAR */
    memcpy(current_prompt, null_line, 80);

} /* gone_prompt */






/****************************************************************
   CALLED WHEN SCROLL OR REVERSE SCROLL. MUST BLANK OUT THE PROMPT
   AND MESSAGE LINES. NOTE: When window_present is TRUE, this should
   never be called
****************************************************************/
void Kill_prompt_and_message() {

    if (macro_exec_level == 0) {
        Move_name(current_prompt, suppressed_prompt);
    }

    Put_start_row(message_line);
    if (output_codes[erase_screen_out_code].code[0] != 0)
        Put_erase_screen();                /* CLEAR MESSAGE AND PROMPT */
    else {
        Put_erase_entire_line();
        Put_start_row(prompt_line);
        Put_erase_entire_line();
    }

    current_prompt[0] = 0;
    current_message[0] = 0;

} /* kill_prompt_and_message */




/*  Initialize the window prompt and message lines to blank lines. */

void Init_prompt_and_message() {

    byte old_row, old_col;

    old_col = col; old_row = row;
    Put_start_row(message_line);
    Put_erase_entire_line();
    Put_start_row(prompt_line);
    Put_erase_entire_line();
    current_message[0] = 0;
    current_prompt[0] = 0;
    Put_goto(old_col, old_row);

} /* init_prompt_and_message */





/****************************************************************
   PRINT PROMPT LINE
    Only if macro_exec_level = 0
****************************************************************/
void Print_prompt(pointer line) {

    if (batch_mode)
        return;

    /* DONT PRINT ANY PROMPTS WITHIN MACROS unless force_writing is on */
    if (macro_exec_level == 0 || force_writing) {
        Rebuild(prompt_line, line, current_prompt);
        Move_name(line, current_prompt);
        /* Set suppressed prompt to current prompt for restoration from inside
           a macro if needed ELSE in a macro ONLY save the line away */
    }
    if (line[0] != 0) /* dont save null lines */
        Move_name(line, suppressed_prompt);

} /* print_prompt */





/****************************************************************
   PRINT PROMPT LINE AND RESTORE CURSOR
****************************************************************/
void Print_prompt_and_repos(pointer line) {
    byte old_row, old_col;

    old_col = col;
    old_row = row;
    Print_prompt(line);
    Clear_count_message();
    Put_goto(old_col, old_row);
} /* print_prompt_and_repos */





/****************************************************************
  Called after scrolling to rewrite the prompt and message lines.
  Only called if window_present=false.
****************************************************************/
void Print_last_prompt_and_msg(boolean small_prompt) {

    Print_message(last_message);
    if (in_input_line) {
        Print_input_line();
    }
    else {
        if (!small_prompt || Not_cursor_movement(last_cmd)
            || suppressed_prompt[0] < 60)
            Print_prompt_and_repos(suppressed_prompt);
    }

} /* print_last_prompt_and_msg */







/***********************************************************************
   CALLED TO ENSURE THAT THE DISPLAY HAS NOT BEEN LOST BECAUSE A MACRO
   WENT OFF OF THE SCREEN.
***********************************************************************/
void Rebuild_screen() {

    byte save_exec_level;
    boolean save_force_writing;

    if (batch_mode) return;
    save_force_writing = force_writing;
    force_writing = _TRUE;
    if (macro_suppress) {
        Co_flush();                        /* EMPTY CO BUFFER */
        Put_home();
        macro_suppress = _FALSE;
        save_exec_level = macro_exec_level;
        macro_exec_level = 0;

        Put_clear_all_text();
        Init_prompt_and_message();
        V_cmnd();               /* FORCE RE-WRITE OF SCREEN */
        Re_view();
        w_dirty = _TRUE;
        Print_last_prompt_and_msg(_FALSE);

        macro_exec_level = save_exec_level;
    }
    else {
        Init_prompt_and_message();
        Print_last_prompt_and_msg(_FALSE);
    }
    force_writing = save_force_writing;

} /* rebuild_screen */

