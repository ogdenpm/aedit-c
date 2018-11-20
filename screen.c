// #SMALL
// #title('SCREEN                     LOW LEVEL SCREEN OUTPUT ROUTINES')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

#include "oscompat.h"
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"

static void Actual_goto(byte goto_col, byte goto_row);

byte str[10];
byte scroll_str[5] = { 4, ESC, 'W', 0, 0 };

logical scrolling = { _FALSE };
/* used to tell insert line to insert on the current
   line rather than the one below it */





static void Print_code(pointer code_p) {

    if (*code_p == 0xff) /* indirection indicator */
        code_p = ((entry_t *)code_p)->p;
    Print_string(code_p);

} /* print_code */




//byte pad[2] = { 1, 0 };
byte msecs_per_pad_char;



static void Delay(word func) {
    word limit;

    if (delay_times[func] == 0)
        return;
    Co_flush();
    /* delay 'func'/2 milliseconds on an 8MZ CPU */
    limit = delay_times[func];
    ms_sleep(limit >> 1);
} /* delay */



byte selecter[4] = { 3, 0x1b, 'M', 0 };    /* USED BY FOLLOWING    */

/*
    SELECT_PARTITION                    FOR SERIES_4 ONLY. SELECTS PARTITION.
                                        MUST BE FOLLOWED BY A GOTO OR HOME
                                        AS CURSOR POSITION IS UNPREDICTABLE.
*/

static void Select_partition(byte part_no) {

    selecter[3] = part_no + ' ';
    Print_string(selecter);
} /* select_partition */


/*
    THE PUT ROUTINES OUTPUT THE CODES OF THE SAME NAME
    ROW AND COL MUST ALWAYS REFLECT REALITY. TOP LEFT OF SCREEN IS 0,0.
*/


static void Put_up() {


    Print_code(output_codes[up_out_code].code);
    row--;
    Delay(up_out_code);
} /* put_up */

static void Put_down() {


    Print_code(output_codes[down_out_code].code);
    row++;
    /*p    CALL delay(down_out_code);*/
} /* put_down */

void Put_left() {
    Print_code(output_codes[left_out_code].code);
    col--;
    Delay(left_out_code);
} /* put_left */

static void Put_right() {
    Print_code(output_codes[right_out_code].code);
    col++;
    Delay(right_out_code);
} /* put_right */

void Put_home() {
    if (output_codes[goto_out_code].code[0] != 0) {
        col = 20; row = 20; /* anything except 0 to force the goto */
        Actual_goto(0, 0);
        return;
    }
    Print_code(output_codes[home_out_code].code);
    row = col = 0;
    Delay(home_out_code);
} /* put_home */

static void Put_back() {
    Print_code(output_codes[back_out_code].code);
    col = 0;
    Delay(back_out_code);
} /* put_back */

void Put_erase_screen() {        /* erase rest of screen function */
    Print_code(output_codes[erase_screen_out_code].code);
    Delay(erase_screen_out_code);
    Gone_prompt();
} /* put_erase_screen */

void Put_erase_entire_screen() {    /* clear screen */
    /*     co_buffer(0)=0;   */               /* abort present output */
    Print_code(output_codes[erase_entire_screen_out_code].code);
    Delay(erase_entire_screen_out_code);
    Put_home();
    Gone_prompt();
} /* put_erase_entire_screen */

void Put_erase_line() {
    if (output_codes[erase_line_out_code].code[0] > 0) {
        Print_code(output_codes[erase_line_out_code].code);
        Delay(erase_line_out_code);
    }
    else {

        boolean no_region;
        no_region = (row == last_text_line && config == VT100);
        if (no_region)
            Reset_scroll_region();
        str[0] = 1;
        str[1] = print_as[' '];
        while (col < line_size[row]) {
            Print_string(str);
            col++;
        }
        Adjust_for_column_80();
        if (wrapper && row >= prompt_line) {
            row = prompt_line;
            /* we scrolled, so do the delay time for the down code */
            Delay(down_out_code);
        }
        if (no_region)
            Put_scroll_region(first_text_line, last_text_line);
    }
} /* put_erase_line */

void Put_erase_entire_line() {
    if (output_codes[erase_entire_line_out_code].code[0] != 0) {
        Print_code(output_codes[erase_entire_line_out_code].code);
        Delay(erase_entire_line_out_code);
    }
    else
        Put_erase_line();
} /* put_erase_entire_line */

static void Put_insert() {
    Print_code(output_codes[reverse_scroll_out_code].code);
    Delay(reverse_scroll_out_code);
} /* put_insert */

static void Put_delete() {
    Print_code(output_codes[delete_out_code].code);
    Delay(delete_out_code);
} /* put_delete */

void Put_normal_video() {
    Print_code(output_codes[normal_video_code].code);
} /* put_normal_video */

void Adjust_for_column_80() {
    if (col == 80) {
        col = 79;
        if (wrapper) {
            row++;
            col = 0;
        }
    }
} /* adjust_for_column_80 */

void Put_reverse_video() {
    Print_code(output_codes[reverse_video_code].code);
} /* put_reverse_video */




static void Actual_goto(byte goto_col, byte goto_row) {
    byte need_row;

    /*    IF SERIES IV THEN MUST EXPLICITLY JUMP BETWEEN PARTITIONS    */

    if (row == goto_row && col == goto_col)
        return;

    need_row = goto_row;

    if (config == VT100 || config == ANSI) {
        byte trow, tcol;
        trow = need_row + 1;
        tcol = goto_col + 1;
        str[0] = 0;
        if (trow > 9)
            str[++str[0]] = (trow / 10) + '0';
        str[++str[0]] = (trow % 10) + '0';
        str[++str[0]] = ';';
        if (tcol > 9)
            str[++str[0]] = (tcol / 10) + '0';
        str[++str[0]] = (tcol % 10) + '0';
        str[++str[0]] = 'H';
    }
    else if (first_coordinate == col_first) {
        str[0] = 2;
        str[1] = goto_col + output_codes[offset_index].code[1]; /* COLUMN NUMBER */
        str[2] = need_row + output_codes[offset_index].code[1]; /* ROW NUMBER */
    }
    else { /* row_first */
        str[0] = 2;
        str[1] = need_row + output_codes[offset_index].code[1]; /* ROW NUMBER */
        str[2] = goto_col + output_codes[offset_index].code[1]; /* COLUMN NUMBER */
    }
    Print_code(output_codes[goto_out_code].code);    /* LEAD IN CODE */
    Print_string(str);

    col = goto_col;
    row = goto_row;
    Delay(goto_out_code);
} /* actual_goto */



/*
    PUT_RE_COL                        SET THE SCREEN COLUMN TO THE DESIRED VALUE
*/

void Put_re_col(byte new_col) {

    if ((int)new_col < (int)col - (int)new_col) {
        Put_back();
    }

    if (new_col < col) {                /* MUST MOVE LEFT */
        if (new_col < (col - new_col)) Put_back();
        else {
            while (new_col < col) {
                Put_left();
            }
            return;
        }
    }
    while (new_col > col) {
        Put_right();
    }
} /* put_re_col */




/*
    PUT_RE_ROW                        SET THE SCREEN ROW TO THE DESIRED VALUE
                                    ONLY USED IF GOTO FUNCTION IS MISSING.
*/

static void Put_re_row(byte new_row, byte want_col) {

    if (want_col < 5 && (int)new_row < (int)row - (int)new_row) {
        Put_home();
    }

    if (new_row < row) {                /* MUST MOVE UP */
        if (output_codes[up_out_code].code[0] != 0) {
            while (new_row < row) {            /* USE CURSOR UP TO CHUG */
                Put_up();
            }
            return;
        }
        Put_home();                        /* MUST HOME THEN CHUG DOWN */
    }

    while (new_row > row) {        /* MUST MOVE DOWN */
        Put_down();
    }
} /* put_re_row */




/*
    PUT_GOTO                        GO TO THE SPECIFIED ROW, COL.
*/
void Put_goto(byte goto_col, byte goto_row) {

    if (output_codes[goto_out_code].code[0] != 0)
        Actual_goto(goto_col, goto_row);
    else {
        Put_re_row(goto_row, goto_col);
        Put_re_col(goto_col);
    }
} /* put_goto */



void Put_normal_video_end() {
    Put_goto(79, row);
    Put_normal_video();
    if (visible_attributes) {
        col = 80;
        Adjust_for_column_80();
    }
} /* put_normal_video_end */


/*
    PUT_START_ROW                    MOVE TO COLUMN ZERO OF THE INDICATED ROW
*/
void Put_start_row(byte new_row) {

    if (new_row == row + 1) {
        Put_down();
    }
    else if (new_row == row) {    /*NULL*/
        ;
    }
    else if (new_row == row - 1) {
        Put_up();
    }
    else if (output_codes[goto_out_code].code[0] != 0) {
        /* else try for absolute cursor positioning */
        Actual_goto(0, new_row);
    }
    else {
        /* other wise use relative positioning as a last resort */
        Put_re_row(new_row, 0);
    }

    if (col > 0)
        Put_back();

} /* put_start_row */



/*
    PUT_CLEAR_ALL                    CLEAR ENTIRE SCREEN. ON SERIES IV THIS
                                    MEANS ALL PARTITIONS.

*/

void Put_clear_all() {
    wbyte i;

    Put_home(); /*    GO HOME FIRST    */

    /*    ERASE ENTIRE SCREEN IF POSSIBLE */
    if (output_codes[erase_entire_screen_out_code].code[0] != 0
        && !window_present) {
        Put_erase_entire_screen();

        /*    ELSE TRY ERASE REST OF SCREEN */
    }
    else if (output_codes[erase_screen_out_code].code[0] != 0 &&
        (!window_present || window_num == 1)) {
        Put_erase_screen();

        /*    ELSE ERASE LINE BY LINE    */
    }
    else {
        for (i = first_text_line; i <= prompt_line; i++) {
            Put_start_row((byte)i);
            Put_erase_entire_line();
        }
    }

    /*    FLAG EVERYTHING AS CLEARED */

    for (i = first_text_line; i <= prompt_line; i++) {
        line_size[i] = 0;
    }

    Put_home();

    Gone_prompt();

} /* put_clear_all */



void Put_clear_all_text() {
    wbyte i;

    if (output_codes[erase_entire_line_out_code].code[0] == 0 &&
        output_codes[erase_line_out_code].code[0] == 0)
        Put_clear_all();
    else {
        if (first_text_line == 0) Put_home();
            else Put_start_row(first_text_line);

        for (i = first_text_line; i <= last_text_line - 1; i++) {
            Put_erase_entire_line();
            Put_down();
            have[i] = 0;
            line_size[i] = 0;
        }

        Put_erase_entire_line();

        line_size[last_text_line] = 0;
        have[last_text_line] = 0;
        if (first_text_line == 0) Put_home();
        else Put_start_row(first_text_line);
    }
} /* put_clear_all_text */



/*    SCROLL_PARTIAL_UP            Scrolls a partial screen area up between
                                current cursor position and last_text_line
*/

static void Scroll_partial_up(byte num) {
    byte i;

    for (i = 1; i <= num; i++) {
        Put_delete();
    }
    Put_start_row(last_text_line - num + 1);
    for (i = 1; i <= num; i++) {
        Put_insert();
    }
} /* scroll_partial_up */






/* SCROLL_PARTIAL_DOWN            Scrolls a partial screen area down between
                                last_text_line and insert_row
*/

static void Scroll_partial_down(byte insert_row, byte num) {

    byte i;

    /*    MUST FIRST DELETE THE LAST LINE SO PROMPT IS PRESERVED    */
    Put_start_row(last_text_line - num + 1);
    for (i = 1; i <= num; i++) {
        Put_delete();
    }
    Put_start_row(insert_row);
    for (i = 1; i <= num; i++) {
        Put_insert();
    }

} /* scroll_partial_down */




/*
    PUT_INSERT_LINE                IF NOT AT END OF SCREEN AND TERMINAL HAS A
                                DELETE LINE FUNCTION THEN INSERT A
                                BLANK LINE IN THE TEXT.
*/

void Put_insert_line(word num) {
    byte insert_row;
    wbyte i;

    if (output_codes[delete_out_code].code[0] != 0
        && row < last_text_line - num) {
        insert_row = row + 1;

        Scroll_partial_down(insert_row, (byte)num);
        /*    SCREEN IS CHANGED. NOW MUST FIX POINTERS    */

        i = message_line;
        while (i > insert_row + num - 1) {
            line_size[i] = line_size[i - num];
            have[i] = have[i - num];
            i--;
        }

        /*    INSERTED LINES ARE BLANK    */

        i = 0;
        while (i < num) {
            line_size[insert_row + i] = 0;
            have[insert_row + i] = 0;
            i++;
        }

    }
} /* put_insert_line */



/*
    PUT_DELETE_LINE                DELETE THE CURRENT ROW. FIX POINTERS.
*/

void Put_delete_line(byte num) {
    wbyte i;
    byte oy;

    oy = row;

    /*    FIX SCREEN POINTERS FIRST    */
    i = row;
    while (i + num <= message_line) {
        line_size[i] = line_size[i + num];
        have[i] = have[i + num];
        i++;
    }

    i = last_text_line - num + 1;
    while (i <= last_text_line) {
        line_size[last_text_line] = 0;
        i++;
    }

    Scroll_partial_up(num);

    Put_start_row(last_text_line - num + 1);

    /*    INSERTED LINE IS BLANK    */

    /*p    line_size(last_text_line)=0;*/

    i = last_text_line - num + 1;
    while (i <= last_text_line) {
        Put_start_row((byte)i);
        have[i + 1] = Print_text_line(have[i]);
        i++;
    }


} /* put_delete_line */



void Put_scroll(byte num_lines) {
    byte i;


    if (output_codes[delete_out_code].code[0] == 0 ||
        output_codes[reverse_scroll_out_code].code[0] == 0) {
        /*
            IF window_present=FALSE THENDO; |* the old code *|
        */
        Put_start_row(prompt_line);
        for (i = 1; i <= num_lines; i++) {
            Print_string("\x1\n");
            Delay(down_out_code);
            line_size[last_text_line] = line_size[message_line];
            line_size[message_line] = line_size[prompt_line];
            line_size[prompt_line] = 0;
        }
        Put_up();
        Put_up();
    }
    else {    /* use insert/delete line functions to partial scroll */
     /* line_size is adjusted in put_delete_line */
        Put_start_row(first_text_line);
        Scroll_partial_up(num_lines);
        Put_start_row(last_text_line);
    }

} /* put_scroll */





void Put_reverse_scroll() {


    if (output_codes[delete_out_code].code[0] == 0 ||
        output_codes[reverse_scroll_out_code].code[0] == 0) {
        /*
            IF window_present=FALSE THENDO; |* the old code *|
        */
        Put_start_row(0);        /* MUST GO TO TOP BEFORE REVERSE SCROLL */
        Put_insert();
    }
    else {
        Put_start_row(first_text_line);
        Scroll_partial_down(first_text_line, 1);
    }
} /* put_reverse_scroll */





void Put_scroll_region(byte first_row, byte last_row) {
    byte str[10];

    if (config == VT100) {

        first_row++;
        last_row++;

        str[0] = 2;
        str[1] = ESC;
        str[2] = '[';

        if (first_row > 9)
            str[++str[0]] = (first_row / 10) + '0';
        str[++str[0]] = (first_row % 10) + '0';

        str[++str[0]] = ';';

        if (last_row > 9)
            str[++str[0]] = (last_row / 10) + '0';
        str[++str[0]] = (last_row % 10) + '0';

        str[++str[0]] = 'r';

        Print_code("\x2" ESCSTR "7");      /* save cursor */
        Print_code(str);              /* VT100 set scroll region */
        Print_code("\x5" ESCSTR "[?6l");   /* VT100 reset origin */
        Print_code("\x2" ESCSTR "8");      /* restore cursor */
    }

} /* put_scroll_region */



void Reset_scroll_region() {

    if (config == VT100 || config == ANSI) {
        Print_code("\x2" ESCSTR "7");      /* save cursor */
        Print_code("\x5" ESCSTR "[1;r");    /* VT100 reset scroll region - 1 added as occasionally not defaulted */
        Print_code("\x2" ESCSTR  "8");      /* restore cursor */
    }

} /* reset_scroll_region */


