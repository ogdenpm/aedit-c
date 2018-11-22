// #SMALL
// #title ('WORDS                     ALTER Word Processing COMMANDS')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"


boolean justify;
    /* Justifying or filling. must be global to keep
       previous value for 'A' command. */


static void Insert_blank() {
    *oa.low_e++ = ' ';
} /* insert_blank */


static void Back_char() {
    Backup_char (1, _FALSE);
} /* back_char */


static void For_char() {
    Forward_char (1, _FALSE);
} /* for_char */


static boolean At_eof() {
    if (oa.high_s >= oa.high_e - 1)
        if (! Can_forward_file())
            return _TRUE;
    return _FALSE;
} /* at_eof */






/******************************************************************
   Position the cursor after the last LF or at the beginning of file.
******************************************************************/
static void Jump_prior_boln() {
    
        pointer ptr;
        word len;

    while (1) {
        len = (word)(oa.low_e - oa.low_s + 10); /* length of memory to be scanned */
        ptr = oa.low_e - len;
        cursor = ptr + findrb(ptr, LF, len) + 1;
        if (cursor == oa.low_s) {
            if (! Backup_file()) {
                Jump_start();
                return;
            /* if backed up then try again */
            /* backup_file rewindowed at the previous low_s */
            }
        } else {
            Re_window();
            return;
        }
    }

} /* jump_prior_boln */





/******************************************************************
   Assumming we are at the beginning of a line, deletes any blanks,
   tabs, and CRs. If the line is null, skips the eoln and returns
   true. If at EOF, inserts a CRLF and returns true. Else returns
   false.
******************************************************************/
static boolean Null_line() {
    
        byte ch; 
        pointer p;

    Set_tag(ed_tagb, oa.high_s);    /* save current location */
    while (1) {
        ch = high_s_byte;
        if (ch == ' ') {
            For_char();
        } else if (ch == CR || ch == LF) {
            For_char();
            p = oa.low_e - 1;
            if (*p == LF) return _TRUE;
        } else if (ch == TAB) {
            For_char();
        } else {
            if (ch == '|') {  /* maybe EOF */
                if (At_eof()) {
                    p = oa.low_e - 1;    /* point to previous byte */
                    if (*p != LF) Insert_crlf();
                    return _TRUE;
                }
            }
            Jump_tag(ed_tagb); /* jump back to beginning of line */
            return _FALSE;
        }
    }

} /* null_line */




/******************************************************************
   Backs up to the beginning of the line. Skips null lines. If at
   EOF, returns false. Else backs up till the closest null line, or
   till the beginning of the file, and returns true.
******************************************************************/
static boolean Find_bop() {

    Jump_prior_boln();
    while (Null_line()) {
        if (At_eof()) {/* eof */
            /* stop macro exec only if we reach eof
               BEFORE beginning of paragraph. */
            Stop_macro();
            return _FALSE;
        }
    }
    while (1) {
        Jump_prior_boln();
        if (oa.low_e <= oa.low_s)
            return _TRUE;  /* start-of-file is same as start-of-parag. */
        if (Null_line())
            return _TRUE;
        Back_char(); /* skip back the LF */
    }

} /* find_bop */






/************************************************************
          Process one line using the the global margin values
          and  justification flag.  Assumes it is pointing to
          the beginning of the line upon entry.
************************************************************/
nestedProc void Chop_blanks(wpointer blanks_count, wpointer blanks_needed, pointer column) {
    while (high_s_byte == ' ') {
        --*blanks_count;
        ++*blanks_needed;
        --*column;
        Back_char();
    }
} /* chop_blanks */


static logical Process_line(byte start_col) {   /* left$margin or indent$margin */
    
    
byte ch, i, column; 
        word blanks_count;     /* number of gaps between words.
                          after periods the gap's length is doubled. */
word blanks_needed;    /* number of blanks needed to justify. */

        byte dir;              /* variables for justify calculations. */

#define gr_one	0
#define le_one	1

        byte rem, ratio, count; 

        byte chopped; 
        boolean was_blank;   /* last inserted, or skipped, char was a blank. */






    /*    first insert the  proper number  of space.  will be
          equivalent to the left margin  or,  if  it  is  the
          first line of the paragraph, the indent margin.  */
    
    column = start_col;    
    for (i = 1; i <= column; i++) {
        Insert_blank();
    }
    Re_window();
    blanks_count = 0;
    was_blank = _TRUE;

    while (column <= right_margin) {
        /* process a line while we are less than the right margin */
        ch = high_s_byte;
        if (ch == ' ') {
            if (was_blank) {
                Forward_char (1, _TRUE);
            } else {
                For_char();
                column++;
                blanks_count++;
                was_blank = _TRUE;
            }
        } else if (ch == CR || ch == TAB) {
            high_s_byte = ' ';
        } else if (ch == LF) {
            /* chop blanks in case that we are at the end of the paragraph */
            chopped = 0;
            oa.low_e = oa.low_e - 1;
            while (low_e_byte == ' ') {
                oa.low_e = oa.low_e - 1;
                chopped++;
            }
            oa.low_e = oa.low_e + 1;

            Insert_crlf();            /* insert a new_crlf */
            Forward_char (1, _TRUE);  /* delete old lf */
            if (Null_line()) {
                while (Null_line()) {
                    if (At_eof()) return _FALSE;
                }
                return _FALSE;   /* EOF, or not a null line. */
            } else {
                Backup_char (1, _TRUE); /* delete the crlf */

                /* restore the chopped blanks */
                for (i = 1; i <= chopped; i++) {
                    Insert_blank();
                }

                Insert_blank();  /* insert a blank instead of the CRLF */
                Back_char();  /* return back to the blank */
            }
        } else if (ch == '.' || ch == '!' || ch == '?') {
            For_char();
            column++;
            was_blank = _FALSE;
            ch = high_s_byte;
            if (ch == ' ' || ch == TAB || ch == CR || ch == LF) {
                for (i = 1; i <= 2; i++) {
                    if (column <= right_margin) {
                        Insert_blank();
                        column++;
                        blanks_count++;
                        was_blank = _TRUE;
                    }
                }
            }
        } else {
            if (ch == '|') {  /* maybe EOF */
                if (At_eof()) {
                    Insert_crlf();
                    return _FALSE;
                }
            }
            For_char();
            column++;
            was_blank = _FALSE;
        }
    }


    blanks_needed = 0;   /* number of blanks needed for justifying */

    /* If the last chars before the right margin are blanks - delete them */
    Back_char();
    Chop_blanks(&blanks_count, &blanks_needed, &column);
    For_char();

    ch = high_s_byte;
    if (ch != ' ' && ch != TAB && ch != CR && ch != LF) {
        /* we are in the middle of a word;
           find the best location for chopping the line. */
        Back_char();
        while (high_s_byte != ' ' && column > start_col) {
            Back_char();
            blanks_needed++;
            column--;
        }
        if (column > start_col) {
            /* high_s is pointing to blank */
            /* substract the blank(s) before the word */
            Chop_blanks(&blanks_count, &blanks_needed, &column);
            /* high_s is pointing to non-blank */
            For_char();
        } else {
            /* the line contains one long word */
            Forward_char (blanks_needed, _FALSE);
            ch = high_s_byte;
            while (ch != ' ' && ch != TAB && ch != CR && ch != LF) {
                if (ch == '|') {  /* maybe EOF */
                    if (At_eof()) {
                        Insert_crlf();
                        return _FALSE;
                    }
                }
                For_char();
                ch = high_s_byte;
            }
        }
    }

    /* delete trailing blanks */
    while (high_s_byte == ' ' || high_s_byte == TAB) {
        Forward_char (1, _TRUE);
    }

    Insert_crlf();

    /* delete old CRLF if there is one, and check for end of paragraph. */
    if (high_s_byte == CR) {
        high_s_byte = ' '; /* force forward_char to delete only one char */
        Forward_char (1, _TRUE);
    }
    if (high_s_byte == LF) {
        Forward_char (1, _TRUE);
        if (Null_line()) {
            while (Null_line()) {
                if (At_eof())
                    return _FALSE;
            }
            return _FALSE;
        }
    }


                           /***** JUSTIFY *****/
    if (! justify)
        return _TRUE;
    if (blanks_needed == 0 || blanks_count == 0)
        return _TRUE;

    /* calculating how many blanks to put between words. */
    if (blanks_needed > blanks_count) {
        dir = gr_one;
        ratio = blanks_needed / blanks_count;
        rem = blanks_needed % blanks_count;
        if (rem > 0)
            rem = blanks_count / rem;
    } else {
        dir = le_one;
        rem = blanks_count / blanks_needed;
        ratio = 0;
    }

    Set_tag(ed_tagb, oa.high_s);    /* save current location */
    Back_char();        /* backup to before the CRLF */
    count = 0;
    while (blanks_needed > 0 && blanks_count > 0) {
        do {                /* backup until we find a word delimiter a LF */
            Back_char();
        } while (!(high_s_byte == ' ' || high_s_byte == LF));

        /* If we Got to the LF, then we are done */
        if (high_s_byte == LF) {
            goto ret_true;
        }
        count++;
        if (count >= rem || dir == gr_one) {
            /* its time to insert 1 or more blanks */
            byte max, i;
            max = 0;
            /* decide how many blanks */
            if (dir == gr_one)
                max = ratio;
            if (count == rem && blanks_needed > max) {
                max++;
                count = 0;   /* reset counter */
            }
            for (i = 1; i <= max; i++) {
                Insert_blank();
                Back_char();
                blanks_needed--;
            }
        }
        blanks_count--;
    }
ret_true:
    Jump_tag(ed_tagb);
    return _TRUE;

} /* process_line */





/************************************************************
                Paragraph formatting command
************************************************************/

void P_cmnd() {
    
        word save_count; 
        byte save_command; 
        word i; byte j;

    if (infinite)
        count = 65535;
    save_command = command;
    save_count = count;

re_p_cmnd:
    if (save_command != 'A') {
        command = Input_command("\x4f"
                RVID "Fill      Justify                      "
                RVID "                                      ");
    }

    count = save_count;
    if (count == 0)
        return;
    if (command == CONTROLC || command == esc_code)
        return;
    if (save_command != 'A') {
        if (command == 'F') {
            justify = _FALSE;
        } else if (command == 'J') {
            justify = _TRUE;
        } else {
            Illegal_command();
            goto re_p_cmnd;
        }
    }

    Set_dirty();
    Working();

    for (i = 1; i <= count; i++) {
        Check_for_keys();
        if (cc_flag || ! Find_bop())
            /* find beginning of paragraph. find_bop
               returns false if we are at EOF */
            break;

        if (have[first_text_line] != 0) {
            Re_view(); /*make sure beginning of paragraph is on the screen*/
            Co_flush();
            if (row == last_text_line && count > 1) {
                V_cmnd();
                psaved_line = _FALSE;
            } else {
                Save_line(row); /* save current row so we can update it */
                psaved_line = _TRUE;
                /* flag rest of screen as wrong */
                for (j = row + 1; j <= last_text_line; j++) {
                    have[j] = 0;
                }
            }
        }

        if (Process_line(indent_margin)) {
            while (Process_line(left_margin)) {
                Check_for_keys();
                if (cc_flag)
                    goto exit_loop;
            }
        }

/*
        j=count_lines(have(message_line),oa.low_e)+1;
        IF j+row > prompt_line-first_text_line AND count>0
            THEN CALL v_cmnd;
*/
    }

  exit_loop:
    Re_view();
    psaved_line = _FALSE;

} /* p_cmnd */







/***********   WINDOWING ***********/



static void Switch_windows() {
    
        byte tmp; 
        word saver1;
        pointer saver2;
        boolean save_w_in_other;


    if (macro_suppress) {
        macro_suppress = _FALSE;
        V_cmnd();
        Re_view();
        macro_suppress = _TRUE;
    }

    /* first switch all the window information */

    tmp = first_text_line;
    first_text_line = window.first_text_line;
    window.first_text_line = tmp;

    tmp = last_text_line;
    last_text_line = window.last_text_line;
    window.last_text_line = tmp;

    tmp = message_line;
    message_line = window.message_line;
    window.message_line = tmp;

    tmp = prompt_line;
    prompt_line = window.prompt_line;
    window.prompt_line = tmp;

    tmp = prompt_number;
    prompt_number = window.prompt_number;
    window.prompt_number = tmp;

    Move_name (current_message, tmp_str);
    Move_name (window.current_message, current_message);
    Move_name (tmp_str, window.current_message);

    Move_name (last_message, tmp_str);
    Move_name (window.last_message, last_message);
    Move_name (tmp_str, window.last_message);

    Move_name (current_prompt, tmp_str);
    Move_name (window.current_prompt, current_prompt);
    Move_name (tmp_str, window.current_prompt);

    Move_name (suppressed_prompt, tmp_str);
    Move_name (window.suppressed_prompt, suppressed_prompt);
    Move_name (tmp_str, window.suppressed_prompt);


    /* change the window number; 0=upper, 1=lower */

    window_num = 1 - window_num;

    /* save current position in current window */
    Set_tag(ed_taga, oa.high_s);
    saver1 = oa.wk1_blocks;
    saver2 = oa.high_s;

    /* now if the other window is on the other buffer, flip the pointers */

    if (in_other != w_in_other) {
        Flip_pointers();
        w_in_other = ~ in_other;

        /* now we must restore ed_taga from 'ob' since we just switch the
           two structures */
        oa.tblock[ed_taga] = ob.tblock[ed_taga];
        oa.toff[ed_taga] = ob.toff[ed_taga];
    }

    /* now jump to the new, saved position in the new window.
       while jumping update tagw even if w_in_other<>in_other
       because we are sure that we are in the file to which
       this tagw belongs. */

    save_w_in_other = w_in_other;
    w_in_other = in_other;

    if (oa.tblock[ed_tagw] != 0xffff)
        Jump_tag(ed_tagw);
        else Jump_start();

    w_in_other = save_w_in_other;


    /* save the other window position in tagw.
       we use the tag value if the other window and the
       current window are in the same file (other or main),
       because the jump_tag updates taga. if the second
       window is not in the same file, we sould not change
       the saved value. */

    if (in_other == w_in_other) {
        oa.tblock[ed_tagw] = oa.tblock[ed_taga];
        oa.toff[ed_tagw] = oa.toff[ed_taga];
    } else {
        oa.tblock[ed_tagw] = saver1;
        oa.toff[ed_tagw].bp = saver2;
    }


    /* if the file we are on was altered, then replot the window from scratch*/
    
    if (w_dirty)
        V_cmnd();
    w_dirty = _FALSE;


    Re_view();

} /* switch_windows */






void W_cmnd() {
    
       boolean save_def, save_input_expected, save_force_writing; 
       byte w_center_line;

    if ((output_codes[erase_entire_line_out_code].code[0] == 0 &&
        output_codes[erase_line_out_code].code[0] == 0)) {

        Error("\x20" "insufficient terminal capability");
        return;
    }

    if (!window_present) {
        if (row < 5 || row > last_text_line - 2 || macro_suppress) {
            Error("\x10" "window too small");
            return;
        }

        window.current_message[0] = 0;
        window.last_message[0] = 0;
        window.current_prompt[0] = 0;
        window.suppressed_prompt[0] = 0;
        window.prompt_number = 0;

        save_force_writing = force_writing;
        force_writing = _TRUE;
/*         save_suppress=macro_suppress;  */
        Print_message("");
        Rebuild_screen();  /* only prompt and message */
        first_text_line = row;
        window.first_text_line = 0;
        window.prompt_line = row - 1;
        window.message_line = row - 2;
        window.last_text_line = row - 3;
        w_in_other = in_other;
        w_center_line = ( window.last_text_line * center_line ) / last_text_line;
        Set_tag(ed_tagw, have[w_center_line]);

        window_present = true;
        window_num = 1;
        Switch_windows();

        Init_prompt_and_message();
        Print_prompt(prompts[prompt_number]);

        save_def = in_macro_def;
        in_macro_def = _FALSE;
        save_input_expected = input_expected_flag;
        Set_input_expected ('-');
        Print_message("");
        if (save_input_expected)
            Set_input_expected ('!');
        in_macro_def = save_def;

        Switch_windows();

/*           macro_suppress=save_suppress;  */
        force_writing = save_force_writing;

    } else {
        save_def = in_macro_def;
        in_macro_def = _FALSE;
        save_input_expected = input_expected_flag;
        Set_input_expected ('-');
        force_writing = _TRUE;
        Print_message("");
        in_macro_def = save_def;
        Switch_windows();
        if (save_input_expected)
            Set_input_expected ('!');
        Print_message("");
        force_writing = _FALSE;
    }

} /* w_cmnd */





void K_cmnd() {        /* kill the windowing */

    if (! window_present)
        return;
    window_present = false;

    if (window_num == 0) /* we were in the upper window */
        last_text_line = window.last_text_line;
    first_text_line = 0;
    message_line = last_text_line + 1;
    prompt_line = last_text_line + 2;

    V_cmnd();
    Print_message (&null_str);

} /* k_cmnd */
