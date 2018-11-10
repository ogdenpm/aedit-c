// #SMALL
// #title ('SETCMD                     THE SET COMMAND')
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
#include <memory.h>

static void Set_autocr();
static void Set_bak_file();
static void Set_case();
static void Set_display();
static void Set_e_delimit();
static void Set_go();
static void Set_highbit();
static void Set_indent();
static void Set_k_token();
static void Set_leftcol();
static void Set_margin();
static void Set_notab();
static void Set_pause();
static void Set_radix();
static void Set_showfind();
static void Set_tabs();
static void Set_viewrow();

dispatch_t s_dispatch[18] = {
    'A', Set_autocr,
    'B', Set_bak_file,
    'C', Set_case,
    'D', Set_display,
    'E', Set_e_delimit,
    'G', Set_go,
    'H', Set_highbit,
    'I', Set_indent,
    'K', Set_k_token,
    'L', Set_leftcol,
    'M', Set_margin,
    'N', Set_notab,
    'P', Set_pause,
    'R', Set_radix,
    'S', Set_showfind,
    'T', Set_tabs,
    'V', Set_viewrow,
     0, 0 };

/*    SET_FROM_MACROFILE IS ON WHEN THE SET CODE IS BEING USED BY A MACROFILE
    READ INSTEAD OF INTERRACTIVELY. SET_IS_OK IS SET FALSE BY A SET ERROR. */

byte set_from_macrofile = { _FALSE };
byte set_is_ok;

pointer set_prompts[] = { "\x4f" RVID "Autonl    Bak_file  Case      Display  "
                                RVID "E_delimit Go        Highbit   --more--",
                        "\x4f" RVID "Indent    K_token   Leftcol   Margin   "
                               RVID "Notab     Pause     Radix     --more--",
                        "\x4f" RVID "Showfind  Tabs      Viewrow            "
                               RVID "                              --more--" };

boolean err;

byte illegal_set_command[] = { "\x13" "illegal set command" };


/*    THE FOLLOWING SET I/O ROUTINES OBTAIN INPUT FROM THE CONSOLE
    OR FROM THE MACROFILE DEPENDING UPON SET_FROM_MACROFILE.    */


    /*
        set_error                PRINT REGULAR OR MACRO FILE MESSAGE
    */

static void Set_error(pointer msg) {

    if (set_from_macrofile) {
        Macro_file_error(msg);
        set_is_ok = _FALSE;
    }
    else {
        Error(msg);
    }
} /* set_error */





/*
    SET_INPUT_YES_NO                SET VERSION OF INPUT_YES_NO
*/

static void Set_input_yes_no(pointer prompt_string, pointer current_value_p) {
    byte ch;

    if (set_from_macrofile) {
        ch = Macro_not_blank();
        if (ch == 'Y') {
            *current_value_p = _TRUE;
        }
        else if (ch == 'N') {
            *current_value_p = _FALSE;
        }
        else {
            Macro_file_error(illegal_set_command);
            set_is_ok = _FALSE;
        }
        return;
    }

    if ((ch = Input_yes_no(prompt_string, *current_value_p)) != CONTROLC)
        *current_value_p = ch;

} /* set_input_yes_no */




/*
    SET_INPUT_LINE                    INPUT A LINE OF TEXT FROM EITHER THE
                                    CONSOLE OR FROM A MACRO FILE.
*/

static byte Set_input_line(pointer prompt, pointer prev_input) {
    byte ch;


    if (set_from_macrofile) {
        input_buffer[0] = 0;
        while ((input_buffer[0] < last(input_buffer) - 1) &&
            ((ch = Macro_not_blank()) != ';')) {
            if (Is_illegal(ch)) {
                Macro_file_error(illegal_set_command);
                set_is_ok = _FALSE;
                return CONTROLC;
            }
            input_buffer[++input_buffer[0]] = ch;
        }
    }
    else {
        ch = Input_line(prompt, prev_input);
    }

    input_buffer[input_buffer[0] + 1] = CR;
    return ch;

} /* set_input_line */




static void Set_autocr() {
    Set_input_yes_no("\x1a" "Insert <nl> automatically?", &crlf_insert);
} /* set_autocr */




static void Set_bak_file() {
    Set_input_yes_no("\x12" "Create .BAK files?", &backup_files);
} /* set_bak_file */




static void Set_case() {
    /* Cheat. Asking about the complement value of the flag. */
    find_uplow = ~find_uplow;
    Set_input_yes_no("\x1d" "Consider case of Find target?", &find_uplow);
    find_uplow = ~find_uplow;
} /* set_case */




/*                                          ALLOWS SETTING OF WHETHER OR NOT
                                            MACRO OUTPUT IS TO BE TURNED
                                            OFF WHEN A MACRO GOES OFF THE
                                            SCREEN
*/
static void Set_display() {
    Set_input_yes_no("\x18" "Display macro execution?", &dont_stop_macro_output);
} /* set_display */




/* SET_DELIMITERS                    Allows setting of the delimiter set.
                                    Delimiters by default are everything
                                    except the alphanumerics, $ and _;
                                    If this routine is invoked, the
                                    delimiter set is changed to all
                                    control characters and all characters
                                    above a tilde PLUS whatever is
                                    given as the argument to the SD call
*/

nestedProc byte Macro_not_blank() {
    byte ch;
    // a_goto:
    do {
        ch = Macro_byte();
    } while (ch == ' ' || ch == LF || ch == TAB);
    return ch;
} /* macro_not_blank */

static void Set_e_delimit() {
    byte ch;
    word i;

    if (set_from_macrofile) {
        input_buffer[0] = 0;
        ch = Macro_not_blank();
        while ((input_buffer[0] <= last(input_buffer)) && (ch != CR)) {
            if (Is_illegal(ch)) {
                Macro_file_error(illegal_set_command);
                set_is_ok = _FALSE;
                return;
            }
            input_buffer[++input_buffer[0]] = ch;
            ch = Macro_not_blank();
        }
    }
    else {
        Init_str(tmp_str, sizeof(tmp_str));
        for (i = 33; i <= 126; i++) {
            if (delimiters[i] == 1)
                Add_str_char((byte)i);
        }
        if (Input_line("\xf" "Delimiter set: ", tmp_str) == CONTROLC)
            return;
    }
    if (input_buffer[0] == 0)
        return;

    /* set up defaults */
    memset(&delimiters[0], 1, 33);      /* set all control characters */
    memset(&delimiters[33], 0, 94);     /* reset all printable chars. */
    memset(&delimiters[127], 1, 129);   /* set everthing above tilde */   
    i = 0;
    while (++i <= input_buffer[0]) {
        delimiters[input_buffer[i]] = 1;
    }
} /* set_e_delimit */




static void Set_go() {
    Set_input_yes_no("\x29" "Continue macro execution after a failure?",
        &go_flag[macro_exec_level]);
} /* set_go */




static void Set_highbit() {

    Set_input_yes_no("\x23" "Display parity-on characters as is?", &highbit_flag);
    V_cmnd();  /* force re_view from scratch */
    if (w_in_other == in_other)
        w_dirty = _TRUE;

} /* set_highbit */




static void Set_indent() {
    Set_input_yes_no("\x26" "Automatically indent during insertion?", &auto_indent);
} /* set_indent */




/*
    SET_TOKEN                    A TRUE FALSE FLAG. IF TRUE, ONLY SEARCH
                                strings surrounded by delimiters are
                                found, else any matching string
*/
static void Set_k_token() {
    Set_input_yes_no("\x18" "Find only token strings?", &token_find);
} /* set_k_token */




static void Set_leftcol() {

    word left; word num;

    left = oa.left_column;
    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_num(left, 10);
    if (Set_input_line("\xd" "Left column: ", tmp_str) == CONTROLC)
        return;

    Init_scan(&input_buffer[1]);
    if (Skip_char(CR))
        return;
    if (Skip_char('+')) {
        num = Num_in(&err);
        err = err | (num > 300); /* 300 - only to prevent ovf */
        left = left + num;
    }
    else if (Skip_char('-')) {
        num = Num_in(&err);
        err = err | (num > 300); /* 300 - only to prevent ovf */
        left = left - num;
    }
    else {
        left = (num = Num_in(&err));
    }

    if (!Skip_char(CR) || err || (left > 175)) {
        Set_error("\xb" "bad Leftcol");
        Stop_macro();
    }
    else {
        oa.left_column = (byte)left;
        V_cmnd();                /* FORCE COMPLETE RE_VIEW */
        if (w_in_other == in_other)
            w_dirty = _TRUE;
    }

} /* set_leftcol */




static void Set_margin() {
    word indent, left, right, num, dummy;
    boolean new_indent, new_left, new_right;
    boolean err1;

    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_num(indent_margin, 10);
    Add_str_str("\x3" " , ");
    Add_str_num(left_margin, 10);
    Add_str_str("\x3"" , ");
    Add_str_num(right_margin, 10);
    if (Set_input_line("\x13" "Indent,Left,Right: ", tmp_str) == CONTROLC)
        return;

    indent = indent_margin;
    left = left_margin;
    right = right_margin;

    err1 = _FALSE;
    new_indent = new_left = new_right = _FALSE;
    Init_scan(&input_buffer[1]);
    if (Skip_char(CR))
        return;
    if (!Skip_char(',')) {
        indent = Num_in(&err);
        err1 = err1 | err;
        new_indent = _TRUE;
        dummy = Skip_char(',');
    }
    if (!Skip_char(CR)) {
        if (!Skip_char(',')) {
            left = Num_in(&err);
            err1 = err1 | err;
            new_left = _TRUE;
            dummy = Skip_char(',');
        }
        if (!Skip_char(CR)) {
            right = Num_in(&err);
            err1 = err1 | err;
            new_right = _TRUE;
            if (!Skip_char(CR))
                err1 = _TRUE;
        }
    }

    if (err1) {
        Set_error("\xb" "bad margins");
        return;
    }

    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_str("\x4" "bad ");

    if (new_indent && (indent > 253 || indent >= right)) {
        Add_str_str("\x6" "indent");
        num = indent;
    }
    else if (new_left && (left > 253 || left >= right)) {
        Add_str_str("\x4" "left");
        num = left;
    }
    else if (new_right && (right == 0 || right > 254 ||
        right <= left || right <= indent)) {
        Add_str_str("\x5" "right");
        num = right;
    }

    if (tmp_str[0] != 4 /* there are errors */) {
        Add_str_str("\x9" " margin: ");
        Add_str_num(num, 10);
        Set_error(tmp_str);
    }
    else {
        indent_margin = (byte)indent;
        left_margin = (byte)left;
        right_margin = (byte)right;
    }

} /* set_margin */





static void Set_notab() {
    Set_input_yes_no("\x17" "Insert blanks for tabs?", &blank_for_tab);
} /* set_notab */




static void Set_pause() {
    Set_input_yes_no("\x18" "Pause before continuing?", &pause);
} /* set_pause */




static void Set_radix() {

    byte ch, ch2;

    byte suffices[] = { 5, 'D', 'H', 'O', 'B', 'A' };
    byte radices[] = { 5, 10, 16,  8,  2,  0 };
    byte set_radix_prompt[] = { "\x4f" RVID "Alpha     Binary    Decimal   Hex      "
                                      RVID "Octal                                 " };
    byte strings[] = {
           "\x7" "Decimal"
           "\x3" "Hex    "
           "\x5" "Octal  "
           "\x6" "Binary "
           "\x5" "Alpha  "
    };

    while (1) {
        if (set_from_macrofile) {
            ch = Macro_not_blank();
        }
        else {
            Init_str(tmp_str, sizeof(tmp_str));
            Add_str_str("\xf" "Current radix: ");
            Add_str_str(&strings[Find_index(radix, radices) * 8]);
            Print_message(tmp_str);
            ch = Input_command(set_radix_prompt);
            if ((ch == CONTROLC) || (ch == esc_code) || (ch == CR)) {
                command = 0; /* if command=CR, the last prompt is freezed */
                return;
            }
        }
        ch2 = Find_index(ch, suffices);
        if (ch2 != 5) {
            radix = radices[ch2 + 1];
            return;
        }
        Set_error("\xd" "illegal radix");
    }
} /* set_radix */





/*  SET_SHOW_FIND                    A TRUE FALSE FLAG. IF TRUE, MULTIPLE
                                    FINDS AND REPLACES CLEAR THE SCREEN
                                    AND LIST THE LINE CONTAIINIG THE MATCH.
*/
static void Set_showfind() {
    Set_input_yes_no("\x1d" "List lines on multiple finds?", &show_find);
} /* set_showfind */




/*
    LIST_TABS                        LIST THE CURRENT TAB SETTINGS
*/
static void List_tabs() {

    byte i, j, dif, lasttab, skipped_i;

    if (set_from_macrofile == _FALSE) {
        skipped_i = dif = lasttab = 0;
        for (i = 0; i <= last(tab_to); i++) {          /* RECONSTRUCT TABS FROM LASTTAB */
            j = tab_to[i] + i;                /* DESTINATION OF TAB    */
            if (j != lasttab) {    /* NEXT TAB SETTING IS DIFFERENT */
                if (j - lasttab != dif && j <= last(tab_to)) {
                    dif = j - lasttab;
                    if (skipped_i != 0) {
                        i = skipped_i - 1;
                        skipped_i = dif = 0;
                    }
                    else {
                        Add_str_num(j, 10);/* THEN REPORT     */
                        Add_str_char(' ');
                    }
                }
                else if (skipped_i == 0) {
                    skipped_i = i;
                }
                lasttab = j;
            }
        }
    }
} /* list_tabs */



/*
    SET_TABS                           SET NEW TAB COLUMNS. MUST RE_VIEW WHEN
                                    DONE TO REFLECT NEW COLUMNS.
*/

nestedProc void Update(wpointer lasttab, wpointer nexttab) {
    while (*lasttab < *nexttab) {
        tab_to[*lasttab] = *nexttab - *lasttab; /* SET NEW TAB_TO VALUES */
        ++*lasttab;
    }
}

static void Set_tabs() {

    byte dif;
    word lasttab, nexttab;
    byte dummy;



    Init_str(tmp_str, sizeof(tmp_str));
    List_tabs();
    if (Set_input_line("\x6" "Tabs: ", tmp_str) == CONTROLC)
        return;

    /* CHECK SYNTAX FIRST */
    Init_scan(&input_buffer[1]);

    if (Skip_char(CR)) return;
    lasttab = 0;
    while (!Skip_char(CR)) {
        nexttab = Num_in(&err);
        if ((nexttab <= lasttab) || (nexttab > 253) || err) {
            Set_error("\x8" "bad tabs");
            return;
        }
        lasttab = nexttab;
        dummy = Skip_char(',');
    }

    /* NOW THE INPUT IS VALID. SET NEW TAB VALUES    */
    Init_scan(&input_buffer[1]);
    lasttab = 0;                            /* START IS ZERO */
    dif = 4;                                /* DEFAULT IS TO TAB BY 4 */
    while (!Skip_char(CR)) {
        nexttab = Num_in(&err);
        dif = nexttab - lasttab;
        Update(&lasttab, &nexttab);
        dummy = Skip_char(',');
    }
    /* complete updating the rest of the array tab_to. */
    while (lasttab <= last(tab_to)) {
        nexttab = Min(lasttab + dif, last(tab_to) + 1);
        Update(&lasttab, &nexttab);
    }

    if (set_from_macrofile) return;
    V_cmnd();                /* FORCE COMPLETE RE_VIEW */
    if (w_in_other == in_other)
        w_dirty = _TRUE;

} /* set_tabs */




/*
    SET_VIEWROW                            ROW OF SCREEN ON WHICH VIEW CENTERS.
                                        DEFAULT IS 5 AND IT MUST BE LESS THAN
                                        OR EQUAL TO TEXT SIZE.
*/
static void Set_viewrow() {

    byte temp;

    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_num(center_line, 10);
    if (Set_input_line("\xe" "Row for View: ", tmp_str) == CONTROLC)
        return;
    Init_scan(&input_buffer[1]);
    if (Skip_char(CR))
        return;
    temp = (byte)Num_in(&err);
    if ((temp > last_text_line && temp > window.last_text_line) || err || !Skip_char(CR)) {
        /* 0, last_text_line, and window.last_text_line were forbidden. IB */
        Set_error("\xe" "bad View row");
    }
    else {
        center_line = temp;
        V_cmnd();                /* FORCE COMPLETE RE_VIEW */
        if (w_in_other == in_other)
            w_dirty = _TRUE;
    }

} /* set_viewrow */












/*
    S_CMND                                SET COMMAND
*/

void S_cmnd() {
    byte ch, set_prompt_number;

    set_prompt_number = 0;
    while (1) {
        if (set_from_macrofile) {
            ch = Macro_not_blank();
        }
        else {
            ch = Input_command(set_prompts[set_prompt_number]);
            while (ch == TAB) {
                set_prompt_number = (set_prompt_number + 1) % length(set_prompts);
                ch = Input_command(set_prompts[set_prompt_number]);
            }
            if ((ch == CONTROLC) || (ch == esc_code)) return;
        }

        if (Dispatched(ch, s_dispatch))
            return;
        if (set_from_macrofile) {
            Set_error(illegal_set_command);
            return;
        }
        else {
            Illegal_command();
        }
    }
} /* s_cmnd */




/*
   CALLED FROM ROUTINE THAT HANDLES MACROFILE WHEN A 'S' FOR SET COMMAND IS
   ENCOUNTERED. SETS SET_FROM_MACROFILE FLAG TRUE AND CALLS S_CMND. RETURNS
   TRUE IF SET COMMAND WAS VALID. IF NOT VALID, ERROR MESSAGE WILL HAVE BEEN
   PRINTED. */

byte Set_from_macro() {

    set_from_macrofile = set_is_ok = _TRUE;  /* FLAG FOR NON-INTERRACTIVE */
    S_cmnd();
    set_from_macrofile = _FALSE;
    return set_is_ok;
} /* set_from_macro */
