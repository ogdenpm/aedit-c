// #SMALL
// #title('FIND                          FIND AND REPLACE')
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

static void Do_replace(byte need_save);


/*    FOLLOWING USED BY FIND AND REPLACE    */

byte target[string_len_plus_1] = { 0 };   /* Find STRING */
byte replacement[string_len_plus_1] = { 0 };  /*REPLACE STRING*/

byte other_target[string_len_plus_1];

byte lines_changed;

dword cnt_fnd = 0;   /* matches counter */
dword cnt_rep = 0;   /* replacements counter */

boolean ignore_failure;



/**************************************************************************/
/**************************************************************************/
static void Print_counters() {

    if (!In_macro_exec()) {
        Init_str(tmp_str, sizeof(tmp_str));
        Add_str("    found: ");
        Add_str_num(cnt_fnd, 10);
        if (command != 'F') { /* R or ? */
            Add_str("    replaced: ");
            Add_str_num(cnt_rep, 10);
        }
        force_writing = _TRUE;
        Print_message(tmp_str);
        force_writing = _FALSE;
    }

} /* print_counters */




/**************************************************************************/
/*                               FR_CMND                                  */
/* Find AND Replace COMMANDs                                              */
/*                                                                        */
/**************************************************************************/
void Fr_cmnd() {

    byte f_row, chug_on, i, j;
    pointer dummy, last_line, next_line;
    byte save_macro_level;
    boolean save_force_writing;
    /*BEGIN*/
        /* A MEANS REPEAT ENTIRE FIND OR REPLACE COMMAND. NO REPROMPT FOR
           ARGUMENTS */
    if (command == 'A') {
        command = last_cmd;
    }
    else {
        /* '-' IS A BACKWARDS FIND */
        if (command == '-') {
            minus_type = _TRUE;
            command = 'F';
        }
        else {
            minus_type = _FALSE;
        }
        if (Input_fr() == CONTROLC) {
            Re_view();
            return;
        }
    }

    Kill_message();
    Co_flush();

    cnt_fnd = 0; /* matches counter */
    cnt_rep = 0; /* replacements counter */

    if (count == 0 && !infinite) return;

    ignore_failure = _FALSE; /* failure in the first search always stops macro exec. */

    Working();

    lstfnd = chug_on = Find();                    /* first FIND */

    ignore_failure = infinite; /* if infinite then don't stop macro exec. */

    /* IF NOT FOUND, PRINT MESSAGE AND QUIT */
    if (!chug_on) {                /* FAILURE */
        if (macro_exec_level == 0) {
            Init_str(tmp_str, sizeof(tmp_str));
            Add_str("not found: \"");
                Add_str_special(target);
            Add_str_char('"');
            Print_message(tmp_str);
        }
        return;
    }

    /* IF REPLACE, THEN TEXT IS CHANGED    */
    if (command != 'F') {
        Set_dirty();

        /* MUST COUNT NUMBER OF LINES THAT WILL BE CHANGED */

        j = lines_changed = 0;
        i = 1;
        while (i <= target[0]) {
            if (target[i] == LF)
                lines_changed++;
            i++;
        }
        i = 1;
        while (i <= replacement[0]) {
            if (replacement[i] == LF)
                j++;
            i++;
        }
        if (j > lines_changed)
            lines_changed = j;
    }

    /* DO REPLACE QUERY HERE    */
    if (command == '?') {
        while (chug_on) {
            Rebuild_screen();       /* IF SCREEN SUPPRESSED, BRING BACK. */
            save_macro_level = macro_exec_level;
            macro_exec_level = 0;
            Re_view();
            macro_exec_level = save_macro_level;
            i = Input_yes_no_from_console("ok to replace?", _FALSE, _TRUE);
            if (Have_controlc()) {
                infinite = _FALSE;
                count = 1;/* FORCE END */
            }
            else if (i) {
                /* HAVE PERMISSION TO DO THE REPLACE */
                Do_replace(_TRUE);
            }
            if (count == 1) {
                chug_on = _FALSE;
            }
            else {
                Working();
                lstfnd = chug_on = Find();
            }
            if (!infinite) count--;
        }

    }
    else if (count != 1 && show_find  && minus_type == _FALSE) {

        /* THE FOLLOWING WILL LIST ALL LINES THAT ARE FOUND */
        /* AFTER THE SUBSTITUTIONS (IF ANY) HAVE BEEN MADE */

        save_force_writing = force_writing;
        force_writing = _TRUE;
        Kill_message();
        Print_prompt(&null_str);
        last_line = 0;                /* USED SO SAME LINE IS ONLY LISTED ONCE */
        f_row = 99;
        while (chug_on) {
            if (f_row >= message_line) {
                /* f_row > message_line only initially to force clear_screen */
                if (f_row == message_line) {
                    if (Hit_space() != ' ') { /* LET THE USER READ */
                        lstfnd = _FALSE;
                        force_writing = save_force_writing;
                        return;
                    }
                }
                f_row = first_text_line;
                Put_clear_all_text();
                Co_flush();
            }
            Do_replace(_FALSE);
            next_line = Backup_line(0, oa.low_e, _FALSE);/* START OF CURRENT LINE */

            if (next_line != last_line && last_line > 0) {  // TODO: fix last_line test to check if not at start of memory
                /*    PRINT THE LAST LINE FOUND */
                Put_start_row(f_row);
                Print_line(saved_bytes);
                Co_flush();
                f_row++;
            }
            last_line = next_line;
            dummy = Unfold(next_line, saved_bytes);
            if (count == 1) {
                chug_on = _FALSE;
            }
            else {
                lstfnd = chug_on = Find();
            }
            if (!infinite) count--;
        }
        /* print the last line found */
        if (f_row == message_line) {
            if (Hit_space() != ' ') { /* LET THE USER READ */
                lstfnd = _FALSE;
                force_writing = save_force_writing;
                return;
            }
            Put_clear_all_text();
            f_row = first_text_line;
        }
        Put_start_row(f_row);
        Print_line(saved_bytes);
        if (Hit_space() != ' ') { /* LET THE USER READ */
            lstfnd = _FALSE;
            force_writing = save_force_writing;
            return;
        }
        force_writing = save_force_writing;
    }
    else {                        /* FIND OR REPLACE WITHOUT DISPLAY */
        Do_replace(_TRUE);
        while (count != 1) {
            if ((lstfnd = Find())) {
                Do_replace(_TRUE);
                if (!infinite)
                    count--;
            }
            else {
                count = 1;
            }
        }
    }
    if (infinite) lstfnd = _TRUE;
    Print_counters();

} /* fr_cmnd */




/**************************************************************************/
/*                              DO_REPLACE                                */
/* DO THE ACTUAL SUBSTITUTION FOR THE REPLACE COMMAND.  IF FLAG IS FALSE, */
/* DO NOT WORRY  ABOUT THE DISPLAY.   IF FLAG IS TRUE,  RE_VIEW IF END OF */
/* CHANGE IS ON THE SCREEN.                                               */
/*                                                                        */
/**************************************************************************/
static void Do_replace(byte need_save) {
    byte i, saved_line;
    pointer temp;
    /*BEGIN*/
    if (command != 'F') {            /* REPLACE, NOT FIND, COMMAND */
        temp = oa.low_e - target[0];        /* START ON FOUND STRING */
        if (oa.high_s >= have[message_line]) {
            need_save = _FALSE;
            if (lines_changed > 0)
                V_cmnd();
        }
        if (need_save) {
            /* FIND THE ROW OF THE LINE NOW BEING CHANGED */
            saved_line = last_text_line;
            while (temp < have[saved_line]) {
                saved_line--;
            }
            /* DO A SAVE_LINE SO IT CAN BE REBUILT */
            Save_line(saved_line);
        }
        /* IF REPLACING MORE THAN 1 LINE, MUST FLAG SUBSEQUENT LINES AS
           WRONG */
        i = first_text_line + 1;
        while (i <= lines_changed && i + saved_line < message_line) {
            have[i + saved_line] = 0;    /* ASSUME NOTHING USEFUL ON SCREEN */
            i++;
        }
        /* DO THE REPLACEMENT */
        oa.low_e = temp;                    /* DO THE DELETE */
        Clean_tags();
        for (i = 1; i <= replacement[0]; i++) {
            low_e_byte = replacement[i]; /* ADD NEW STRING */
            oa.low_e = oa.low_e + 1;
        }
        Check_window(0);
        if (need_save)
            Re_view();
    }
    cnt_rep++;
} /* do_replace */





/*
;    FIND                FIND A STRING IN THE TEXT. USED
;                    BY Find AND Replace COMMANDS. THE
;                    TARGET STRING IS IN TARGET. IF
;                    FIND_UPLOW IS TRUE, CASE SHOULD BE
;                    IGNORED. IF A MATCH IS FOUND, FIND
;                    REWINDOWS AT END OF FOUND STRING AND
;                    RETURNS TRUE. ELSE IT RETURNS FALSE.
;                    A FAILED FIND WILL RETURN TO THE
;                    STARTING LOCATION IF FORWARDING THE
;                    FILE WAS NECESSARY. CAUTION: USES
;                    ED_TAGA AND ASSUMES IT IS 5.
;
;                    IF THE MINUS_TYPE FLAG IS ON THEN
;                    THE FIND GOES BACKWARDS.
*/

logical Find() {

    word locs;
    pointer match_char_p, old_p;
    byte previous_ch;
    word i;
    byte ch;
    word bytes_to_match;


    /* IF control-c was pressed or null target, abort search, return
       not found
    */
    if (cc_flag == _TRUE || target[0] == 0) {
        Stop_macro();
        return _FALSE;
    }

    /*    SET ED_TAGA IN CASE THE FIND FAILS AND THE FILE IS SKIPPED FORWARD */
    Set_tag(ed_taga, oa.high_s);

    /*    IF FIND_UPLOW THEN OTHER IS THE SAME AS TARGET EXCEPT WITH REVERSE CASE
        IF NOT FIND_UPLOW, OTHER IS SET EQUAL TO TARGET */

    if (find_uplow) {
        for (i = 1; i <= target[0]; i++) {
            ch = target[i];
            if (ch >= 'A' && ch <= 'Z') {
                other_target[i] = ch + 0x20;
            }
            else if (ch >= 'a' && ch <= 'z') {
                other_target[i] = ch - 0x20;
            }
            else {
                other_target[i] = ch;
            }
        }
        other_target[0] = target[0];
    }
    else {
        Move_name(target, other_target);
    }


    if (!minus_type) {

        previous_ch = 0;
        /* helps to consider as a token the part of a non_token
           on which the cursor is positioned, between the cursor
           and the next delimiter. */

//   forward_search:
        while (_TRUE) {
            if (oa.high_e <= oa.high_s + target[0])
                goto none_in_block;

            if (oa.high_s + target[0] < oa.high_s) /* overflow */
                goto none_in_block;

            locs = (word)(oa.high_e - oa.high_s - target[0]);
            /* now locs is the number of possible starting locations */
    /*
            IF token_find=TRUE
                THEN locs=locs-1;
    */
            match_char_p = oa.high_s;

            while (locs > 0) {
                if (*match_char_p == target[1] || *match_char_p == other_target[1]) {
                    old_p = match_char_p;
                    match_char_p++;
                    if (token_find == _TRUE) {
                        if (delimiters[previous_ch] == 0) {
                            previous_ch = *old_p;    /* save p_ch for next
                                                            iteration */
                            locs--;
                            continue;
                        }
                    }
                    bytes_to_match = target[0];
                    i = 2;
                    while (_TRUE) {
                        if (--bytes_to_match == 0) {    /* matched! */
                            if (token_find == _TRUE) {
                                /* see if the next char in memory is a delimiter */
                                /* *match_char_p is already pointing at the next char */
                                if (delimiters[*match_char_p] == 0 &&
                                    (match_char_p < oa.high_e - 1 || Can_forward_file()))
                                    goto not_a_match;
                            }
                            cursor = match_char_p;
                            Re_window();
                            cnt_fnd++;
                            return _TRUE;
                        }
                        if (*match_char_p == target[i] || *match_char_p == other_target[i]) {
                            i++;
                            match_char_p++;
                            continue;
                        }
                        /* match failure - begin searching for first matching char
                            again
                        */
                    not_a_match:
                        match_char_p = old_p;
                        break;
                    }
                }
                previous_ch = *match_char_p;
                match_char_p++;
                locs--;
            }

        none_in_block:
            /* see if we have another block */
            /* first test for control-c */
            Check_for_keys();
            if (cc_flag == _TRUE || Forward_file() == _FALSE || cc_flag == _TRUE)
                break;
            cursor = oa.low_e - target[0];
            Re_window();
        }
    }
    else {    /* backward search */
        word bytes_to_match;
//    backward_search:
        while (_TRUE) {
            if (oa.low_e < target[0] + oa.low_s) goto none_in_back_block;
            match_char_p = oa.low_e - target[0];
            locs = (word)(match_char_p - oa.low_s + 1);
            /* now locs is the number of possible starting locations */

            if (locs == 0) goto none_in_back_block; /* redundant ? */
    /*
            IF token_find=TRUE THEN locs=locs-1;
    */

            while (locs > 0) {
                if (*match_char_p == target[1] || *match_char_p == other_target[1]) {
                    old_p = match_char_p;
                    match_char_p++;
                    if (token_find == _TRUE) {
                        /* check char before string in memory */
                        if (delimiters[old_p[-1]] == 0) {
                            match_char_p = old_p - 1;
                            locs--;
                            continue;
                        }
                    }
                    bytes_to_match = target[0];
                    i = 2;
                    low_e_byte = LF;
                    /* helps to consider as a token the part of a non_token
                       on which the cursor is positioned, between the cursor
                       and the previous delimiter. */

                    while (_TRUE) {
                        if (--bytes_to_match == 0) { /*  matched! */
                            if (token_find == _TRUE) {
                                /* check char after string in memory */
                                /* *match_char_p is already pointing to the next character */
                                if (delimiters[*match_char_p] == 0) {
                                    goto not_a_back_match;
                                }
                            }
                            cursor = old_p;
                            Re_window();
                            cnt_fnd++;
                            return _TRUE;
                        }
                        if (*match_char_p == target[i] || *match_char_p == other_target[i]) {
                            i++;
                            match_char_p++;
                            continue;
                        }
                    not_a_back_match:
                        match_char_p = old_p;
                        break;
                    }
                }
                match_char_p--;
                locs--;
            }

        none_in_back_block:
            /* see if we have another block */
            /* first test for control-c */
            Check_for_keys();
            if (cc_flag == _TRUE || Backup_file() == 0 || cc_flag == _TRUE)
                break;
            cursor = oa.high_s + target[0] - 1;
            Re_window();
        }
    }
// no_match_at_all:
    Jump_tag(ed_taga);
    if (!ignore_failure && !go_flag[macro_exec_level])
        Stop_macro();
    return _FALSE;
} /* find */
