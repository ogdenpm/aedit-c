// #SMALL
// #title ('CMNDS                     THE ALTER COMMANDS')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/
#include <stdbool.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"


static void xx_controlc();
static void Do_delete();
static void Do_delete_line();
static void Do_delete_left();
static void Do_delete_right();
static void U_restore();
static void Xx_rubout();

byte invalid_hex[] = { "\x11" "invalid hex value" };

byte illegal_value[] = { "\xd" "illegal value" };


byte have_slash;        /* USED BY /I COMMAND */

dispatch_t  i_dispatch[6] = {
    delete_code, &Do_delete,
    delete_line_code, &Do_delete_line,
    delete_left_code, &Do_delete_left,
    delete_right_code, &Do_delete_right,
    undo_code, &U_restore,
    0, 0 };

byte ubuf[ubuf_size];
byte ubuf_left = 0;
byte ubuf_used = 0;
byte u_is_line = _FALSE;
pointer u_low_e, u_high_s;

#define xbuf_size	100


dispatch_t x_dispatch[6] = {
       delete_code, &Do_delete,
       rubout_code, &Xx_rubout,
       delete_right_code, &Delete_right_cmnd,
       CONTROLC, &xx_controlc,
       0, 0 };

dispatch_t x_reset_dispatch[4] = {
       delete_line_code, &Delete_line_cmnd,
       delete_left_code, &Delete_left_cmnd,
       undo_code, &Undo_cmnd,
       0, 0 };

byte xbuf[xbuf_size];
word xbuf_added; /* number of chars added after previous end of line. */
byte xbuf_index; /* number of replaced chars. */





/************************************************************************
     Dispatch  the command.  Arguments are  the command, and a dispatch
     table  whose entries are pairs of command and a PROC address.
     If a  command cannot be found then  return false; else execute its
     associated PROC.  (was plmonly)
************************************************************************/
byte Dispatched(byte command, dispatch_t *table_p) {
    byte ch;
    while ((ch = table_p->_char) > 0) {
        if (command == ch) {
            table_p->cmnd();
            return _TRUE;
        }
        table_p++;
    }
    return _FALSE;
} /* dispatched */



/*
    RESET_COLUMN                USED AFTER BACKUP_LINE OR FORWARD_LINE
                                OR DELETE LINE TO RE_WINDOW AT AS NEAR TO
                                THE VIRTUAL COLUMN     AS POSSIBLE. ON ENTRY,
                                MUST BE WINDOWED AT THE START OF THE LINE.
*/
static void Reset_column() {
    byte i, rc_col;

    if (*cursor == LF)     /* STUCK ON LF */
        return;
    rc_col = 0;
    while (rc_col < virtual_col) {
        if (*cursor == LF) {    /* STOP ON LF */
            if (cursor == oa.high_e && Can_forward_file()) {
                /* AT END OF TEXT IN MEMORY */
                /* IF MORE DATA, CONTINUE */
                i = Forward_file();
            }
            else {
                rc_col = 255;                /* FORCE STOP */
                if (cursor == oa.high_s) {
                    cursor = oa.low_e - 1;
                    if (*cursor != CR)
                        cursor = oa.high_s;
                }
                else {
                    cursor--;    /* POINT TO FIRST BEFORE LF */
                    if (*cursor != CR && cursor + 1 != oa.high_e)
                        cursor++;
                }
            }
        }
        else {                        /* NORMAL FORWARD MOVE */
            if (*cursor == TAB)
                rc_col += tab_to[rc_col];
            else
                rc_col++;
            cursor++;
        }
    }
    if (*cursor == LF) {
        cursor--;
        if (*cursor != CR && cursor + 1 != oa.high_e)
            cursor++;
    }
    Re_window();

} /* reset_column */




/*    SET_DIRTY                                Set the dirty flags appropriately
*/



void Set_dirty() {
    oa.dirty = _TRUE;
    if (w_in_other == in_other)
        w_dirty = _TRUE;
} /* set_dirty */





/*
    DONT_SCROLL                    CALLED BY VARIOUS DELETE TYPE FUNCTIONS TO
                                ENSURE THAT DELETING THE LAST LINE DOES NOT
                                FOOL ALTER INTO SCROLLING.
*/

void Dont_scroll() {

    /*    MIGHT HAVE TO LIE ABOUT DISPLAY TO AVOID SCROLLING */
    if (have[message_line] <= oa.high_s)
        have[message_line] = (pointer)(-1);
} /* dont_scroll */





/*
    TEST_CRLF                    INSERT A LINE IF POSSIBLE. CHECK WINDOW.
*/

void Test_crlf() {

    /* IF TERMINAL HAS INSERT FUNCTION THEN USE IT TO MAKE ROOM    */
    Put_insert_line(1);
    if (row == last_text_line)
        have[message_line] = oa.high_s;
    Check_window(0);    /* EXPAND WINDOW IF NECESSARY */
} /* test_crlf */





/*    THE UBUF CONTAINS UP TO 100 CHARACTERS DELETED BY THE LAST */
/*    CONTROL Z, CONTROL A, OR CONTROL X. THE CONTROL U COMMAND  */
/*    (ISSUED IN COMMAND, XCHANGE OR INSERT MODE) RETRIEVES THE  */
/*    DELETION.                                                   */

/*
    UBUF_CHAR                    ADD A CHARACTER TO UBUF.
*/

static void Ubuf_char(byte ch) {
    if (ubuf_used < last(ubuf)) {
        ubuf[ubuf_used] = ch;
        ubuf_used++;
    }
} /* ubuf_char */




/*
    U_SETUP                        REMEMBER THE CURRENT SETTING OF LOW_E
                                AND HIGH_S SO A SUBSEQUENT U_SAVE CAN FILL
                                THE UBUF.
*/

static void U_setup() {

    u_low_e = oa.low_e;
    u_high_s = oa.high_s;
} /* u_setup */




/*
    U_SAVE                        SAVE ANY CHARACTERS DELETED SINCE U_SETUP
                                CALL IN UBUF. (THIS IS NOT AN ADVERTISEMENT)
*/

static void U_save() {

    u_is_line = _FALSE;                    /* ASSUME NOT A DELETE LINE */
    cursor = oa.low_e;
    ubuf_used = 0;
    while (cursor < u_low_e) {            /* SAVE LEFT PART OF LINE    */
        Ubuf_char(*cursor++);
    }
    ubuf_left = ubuf_used;

    cursor = u_high_s;                    /* SAVE RIGHT PART OF LINE */
    while (cursor < oa.high_s) {
        Ubuf_char(*cursor++);
    }
} /* u_save */




/*
    U_RESTORE                    PUT THE STUFF IN THE UBUF BACK INTO THE TEXT.
*/

static void U_restore() {
    word i;

    /*    IF UBUF WAS FILLED BY A DELETE LINE THEN JUMP TO THE START OF THE
        LINE BEFORE PUTTING TEXT BACK    */

    if (u_is_line) {
        cursor = have[row];
        Re_window();
        Save_line(row);
        Test_crlf();
    }
    i = 0;
    while (i < ubuf_left) {            /* RESTORE LEFT PART OF LINE */
        low_e_byte = ubuf[i];
        oa.low_e = oa.low_e + 1;
        i++;
    }

    i = ubuf_used;
    while (i > ubuf_left) {            /* RESTORE RIGHT PART OF LINE */
        i--;
        oa.high_s = oa.high_s - 1;
        high_s_byte = ubuf[i];
    }
} /* u_restore */




/*
    DO_DELETE                    DELETE COUNT NUMBER OF CHARACTERS
                                ON RIGHT OF CURSOR. COUNT WILL ALWAYS BE IN
                                RANGE OF 1 TO 32.
*/
static void Do_delete() {
    Forward_char(count, _TRUE);
    Dont_scroll();
} /* do_delete */




/*
    DO_DELETE_LEFT                DELETE THE PORTION OF THE CURRENT LINE THAT
                                IS TO THE LEFT OF THE CURSOR.
*/

static void Do_delete_left() {

    U_setup();
    /* DELETE FIRST PART OF LINE */
    if (oa.low_e > have[row]) oa.low_e = have[row];
    U_save();
    Clean_tags();                        /* CLEANUP THE TAGS */
} /* do_delete_left */






/*
    DO_DELETE_LINE                DELETE THE CURRENT LINE.
*/

static void Do_delete_line() {

    U_setup();                                /* REMEMBER FOR CONTROL U */
    oa.high_s = have[row + 1];                        /* DELETE REST OF LINE */
    if (oa.high_s >= oa.high_e) {
        oa.high_s = oa.high_e - 1;
        Stop_macro();
    }
    if (oa.low_e > have[row]) oa.low_e = have[row];
    Clean_tags();                        /* CLEANUP THE TAGS */
    U_save();
    u_is_line = _TRUE;

    /*    WANT TO GO TO THE SAME COLUMN OF THE NEXT LINE    */

    cursor = oa.high_s;
    Reset_column();
    force_column = col;
    Dont_scroll();
} /* do_delete_line */




/*
    DO_DELETE_RIGHT                DELETE TO THE END OF THE CURRENT LINE. DO
                                NOT INCLUDE LINE FEED (AND CARRIAGE RETURN
                                IF ANY).
*/

static void Do_delete_right() {
    byte delete_more;

    U_setup();
    delete_more = _TRUE;
    while (delete_more) {
        if (high_s_byte == LF) {
            delete_more = _FALSE;
        }
        else if (high_s_byte == CR) {
            oa.high_s = oa.high_s + 1;
            if (high_s_byte == LF) {
                oa.high_s = oa.high_s - 1;
                delete_more = _FALSE;
            }
        }
        else {
            oa.high_s = oa.high_s + 1;
        }
    }
    if (oa.high_s >= oa.high_e)
        oa.high_s = oa.high_e - 1;
    U_save();
    Clean_tags();                        /* CLEANUP THE TAGS */

} /* do_delete_right */




/*
    insert$a$char                    STICK A CHAR INTO THE TEXT.
*/

void Insert_a_char(byte ch) {

    low_e_byte = ch;
    oa.low_e = oa.low_e + 1;
} /* insert_a_char */





void Insert_crlf() {
#ifdef MSDOS
    Insert_a_char(CR);
#endif
    Insert_a_char(LF);
} /* insert_crlf */




/* INSERT CRLF. TEST FOR AND DO INDENTATION FOR THE INSERT COMMAND. */

static void Crlf_and_indent() {

    /* IF AUTO INDENTING, CHECK FOR THE CASE WHERE THE USER IS INSERTING
        A BLANK LINE. IF TRUE REMOVE WHITE SPACE AND MOVE IT UP TO FORM
        THE AUTO INDENTATION FOR THE NEXT LINE. */

    if (auto_indent) {
        cursor = oa.low_e - 1;
        while (*cursor == ' ' || *cursor == TAB) {
            cursor--;
        }

        if (*cursor == LF) {
            /* THE LINE HAS ONLY BLANKS. MOVE WHITE SPACE UP FOR NEXT LINE */
            cursor++;
#ifdef MSDOS
            memcpy(cursor + 2, cursor, oa.low_e - cursor);
            oa.low_e = oa.low_e + 2;
            *(wpointer)cursor = 0xa0d;  /* INSERT NEW CRLF */
#else
            memcpy(cursor + 1, cursor, oa.low_e - cursor);
            oa.low_e = oa.low_e + 1;
            *cursor = LF;  /* INSERT NEW LF */
#endif
        }
        else {                            /* NOT A NULL LINE */
            Insert_crlf();
            cursor = Backup_line(1, oa.low_e, _FALSE);/* FIND START OF PRIOR LINE */
            while ((*cursor == ' ' || *cursor == TAB) && cursor < oa.low_e) {
                low_e_byte = *cursor;
                oa.low_e = oa.low_e + 1;
                cursor++;
            }
        }
    }
    else {     /* NO AUTO INDENT - JUST STICK IN THE CR AND LF    */
        Insert_crlf();
    }
    Test_crlf();

} /* crlf_and_indent */




/*
    REMOVE_SLASH                REMOVES THE LF INSERTED IN THE TEXT BY THE
                                / I COMMAND.
*/

static void Remove_slash() {

    if (have_slash) {            /* FLAG ON MEANS HAVE THE LF    */
        have_slash = _FALSE;
        Save_line(row);
        oa.high_s = oa.high_s + 1;
        Dont_scroll();
        Re_view();
    }
} /* remove_slash */




/*
    JUMP_END                    JUMP TO END OF FILE
*/
static void Jump_end() {
    while (Forward_file())
        ;
    cursor = oa.high_e - 1;
    Re_window();
} /* jump_end */



/*
    JUMP_START                    JUMP TO START OF FILE
*/
void Jump_start() {
    while (Backup_file())
        ;
    cursor = oa.low_s;
    Re_window();
} /* jump_start */


byte last_move_command = 0; /* 0 = undefined */

/*
    MOVE_CURSOR                        MOVE CHARACTER. MOVE CURSOR AND RE_WINDOW.
*/
static void Move_cursor(byte ch) {
    pointer start;

    /*    THE HOME CODE DEPENDS ON PRIOR COMMAND */
    /*    IF UP OR DOWN - PAGES IS INDICATED DIRECTION    */
    /*    IF RIGHT OR LEFT - JUMPS TO INDICATED END OF LINE */
    /*    IF OTHER - JUMPS TO END OF LINE    */
    /*    HOWEVER, IF AT END OF LINE - JUMPS TO START OF LINE */

    if (ch == home_code) {
        if (last_move_command == 0) {    /* 0 = undefined */
            AeditBeep();
            return;
        }
        if (last_move_command == up_code || last_move_command == down_code) {

            Working();

            /* MOVE PAGE UP OR DOWN */
            force_column = col;
            if (infinite) {    /* infinite - special case */
                if (last_move_command == up_code) {
                    Jump_start();
                }
                else {            /* JUMP TO LAST LINE */
                    Jump_end();
                    cursor = Backup_line(0, oa.low_e, _TRUE);
                }
                Reset_column(); /* FIX THE COLUMN. */
                return;
            }
            count = count * (message_line - first_text_line);
            if (last_move_command == up_code) {
                if (count > 0)
                    cursor = Backup_line(count, oa.low_e, _TRUE);    /* BACKUP PAGES */
            }
            else {
                if (count > 0) Forward_line(count);    /* FORWARD PAGES */
                if (cursor + 1 == oa.high_e)
                    cursor = Backup_line(0, oa.low_e, _TRUE);
            }
            Reset_column(); /* FIX THE COLUMN. */
            /* IF ALREADY AT START OR END OF FILE, DONT RE-WRITE */
            if (oa.high_s < have[first_text_line] || oa.high_s >=
                have[message_line]) {/* START OVER WITH A NEW SCREEN */
                start = Backup_line(row - first_text_line, oa.low_e, _FALSE);
                Put_clear_all_text();
                Re_write(start);
                first_at_sign_is_here = _FALSE;
            }
        }
        else {
            if (last_move_command == left_code) {
                cursor = have[row];
            }
            else if (last_move_command == right_code) {
                /* move to end of current line */
                cursor = have[row + 1] - 2;
                if (*cursor != CR) {
                    cursor++;
                    if (*cursor != LF) cursor++;
                }
                if (cursor < oa.high_s) cursor = oa.high_s;
                if (cursor >= oa.high_e) cursor = oa.high_e - 1;
            }
            Re_window();
        }
        return;
    }

    last_move_command = ch;

    switch (ch - up_code) {

    case 0:    /* UP */
        force_column = col;            /* KEEP CURSOR IN SAME COLUMN */
        if (infinite) {
            Jump_start();
            Reset_column();
        }
        else {
            if (count > 0) {
                cursor = Backup_line(count, oa.low_e, _TRUE);
                Reset_column();
            }
        }

        break;


    case 1:    /* DOWN */
        force_column = col;
        if (infinite) {    /* GOTO FIRST COLUMN OF LAST LINE */
            Jump_end();
            cursor = Backup_line(0, oa.low_e, _TRUE);
            Reset_column();
        }
        else {
            pointer tmp;
            if (count > 0) {
                Forward_line(count);
                if (cursor + 1 == oa.high_e) {
                    cursor = Backup_line(0, oa.low_e, _TRUE);
                    Reset_column();
                }
                else {
                    tmp = cursor - 1;
                    if (*tmp == LF)
                        Reset_column();
                }
            }
        }

        break;


    case 2:     /* LEFT */
        if (infinite) {
            Jump_start();
        }
        else if (count != 0) {
            Backup_char(count, _FALSE);
        }

        break;


    case 3:     /* RIGHT */
        if (infinite) {
            Jump_end();
        }
        else if (count != 0) {
            Forward_char(count, _FALSE);
        }


        break;


    }

} /* move_cursor */




/*
    XBUF_CLEAN                    EMPTY THE X BUFFER
*/
static void Xbuf_clean() {
    xbuf_added = xbuf_index = 0;
    Set_tagi_at_lowe();
} /* xbuf_clean */




/*
    XX_RUBOUT                    DELETE ONE CHARACTER TO THE LEFT OF THE
                                CURSOR FOR XCHANGE COMMAND. IF NO CHARACTERS
                                HAVE BEEN EXCHANGED, DO A LEFT CURSOR.
*/
static void Xx_rubout() {
    if (xbuf_added > 0) {
        Backup_char(1, _TRUE);
        xbuf_added--;
    }
    else if (xbuf_index > 0) {
        Backup_char(1, _TRUE);
        xbuf_index--;
        oa.high_s = oa.high_s - 1;
        high_s_byte = xbuf[xbuf_index];
    }
    else {
        /*    JUST DO A LEFT CURSOR. THIS DOES NOT CHANGE THE TEXT */
        if (have[row] == oa.low_e) have[row] = oa.high_s;
        Move_cursor(left_code);
        Xbuf_clean();
        return;
    }

    /*    WHEN BACKING UP TO THE START OF A LINE, THE HAVE(ROW) POINTER
        GETS MOVED FROM LOW_E TO HIGH_S. FIX. UNLESS ROW IS NO LONGER SAVED ROW */
    if (have[row] == oa.high_s && saved_from == oa.low_e)
        have[row] = oa.low_e;
} /* xx_rubout */



/*
    xx_controlc                    DELETE ALL NEW XCHANGE CHARACTERS PRIOR
                                TO CURSOR ON CURRENT LINE.
*/
static void xx_controlc() {

    while (xbuf_added + xbuf_index > 0) {
        Xx_rubout();
    }
} /* xx_controlc */




/*
    CR_CMND                            CARRIAGE RETURN MEANS GO TO FIRST COLUMN OF
                                    NEXT LINE. IF AUTO_INDENT IS ON, MOVE TO
                                    FIRST NON BLANK CHARACTER.
*/

void Cr_cmnd() {

    if (infinite)
        Jump_end();
    else
        Forward_line(count);
    if (auto_indent) {
        while (high_s_byte == ' ' || high_s_byte == TAB) {
            Forward_char(1, _FALSE);
        }
    }
} /* cr_cmnd */




/*
    TAB_CMND                        A TAB ROTATES THE PROMPT AND MOVES THE
                                    CURSOR TO A VALID POSITION.
*/


byte prompt_number = { 0 };
void Tab_cmnd() {

    prompt_number = (prompt_number + 1) % 4;
} /* tab_cmnd */



/*
    DELETE_CMND                    DELETE COUNT CHARACTERS
*/
void Delete_cmnd() {

    /*    MAXIMUM COUNT IS 32    */

    if (count == 0)
        return;

    if (infinite || count > 32) {
        Error("\x1a" "cannot delete more than 32");
        return;
    }
    Set_dirty();
    Save_line(row);
    Do_delete();
} /* delete_cmnd */




/*
    DELETE_LINE_CMND                DELETE THE CURRENT LINE.
*/

void Delete_line_cmnd() {

    Set_dirty();
    Save_line(row);
    Do_delete_line();
} /* delete_line_cmnd */




/*
    DELETE_LEFT_CMND                DELETE TO THE LEFT OF THE CURRENT
                                    CURSOR POSITION.
*/

void Delete_left_cmnd() {

    Set_dirty();
    Save_line(row);
    Do_delete_left();
} /* delete_left_cmnd */




/*
    DELETE_RIGHT_CMND                DELETE TO THE RIGHT OF THE CURRENT CURSOR.
*/

void Delete_right_cmnd() {

    Set_dirty();
    Save_line(row);
    Do_delete_right();
} /* delete_right_cmnd */




/*
    MOVE_CMND                        DO THE FIVE MOVE OPERATIONS-
                                    UP, DOWM, LEFT, RIGHT, HOME.
*/
byte Move_cmnd(byte ch) {

    if (ch >= up_code && ch <= home_code) {
        Move_cursor(ch);                /* MOVES AND REWINDOWS */
        return _TRUE;
    }
    return _FALSE;
} /* move_cmnd */




/*
    RUBOUT_CMND                    RUBOUT ONE CHARACTER.
*/

void Rubout_cmnd() {
    Set_dirty();
    Save_line(row);
    Backup_char(1, _TRUE);
} /* rubout_cmnd */




/*
    UNDO_CMND                    RESTORE THE TEXT DELETED BY THE LAST
                                DELETE_LINE OR DELETE_LEFT OR DELETE_RIGHT.
*/

void Undo_cmnd() {

    Set_dirty();
    Save_line(row);
    U_restore();
} /* undo_cmnd */




/*
    HEX_VALUE                    TRIVIAL UTILITY FOR CONVERTING A HEX DIGIT
                                TO BINARY VALUE. RETURNS NOT_HEX IF NOT VALID.
*/

byte Hex_value(byte _char) {
    if (_char >= '0' && _char <= '9') return _char - '0';
    if (_char >= 'A' && _char <= 'F') return _char - ('A' - 10);
    if (_char >= 'a' && _char <= 'f') return _char - ('a' - 10);
    return not_hex;
} /* hex_value */





/* SEE IF INPUT_BUFFER IS A VALID HEX STRING. */

static boolean Convert_string_to_hex() {
    byte i, h, len;

    input_buffer[input_buffer[0] + 1] = CR;
    Init_scan(&input_buffer[1]);
    len = 0;
    while (!Skip_char(CR)) {
        i = 0;
        while ((h = Hex_value(scan_byte)) != not_hex) {
            if (++i & 1) {  /* odd */
                input_buffer[++len] = (byte)(h << 4);
            }
            else {                 /* even */
                input_buffer[len] += h;
            }
            scan_p++;
        }
        if (scan_byte != ' ' && scan_byte != CR) return _FALSE;
        if (i == 1) {
            /* we interpreted the single char improperly. */
            input_buffer[len] >>= 4;
        }
        else if ((i & 1) != 0) { /* longer then 1 and not even */
            return _FALSE;
        }
    }
    input_buffer[0] = len;
    return _TRUE;

} /* convert_string_to_hex */





byte s_hex[string_len_plus_1] = { 0 };


byte last_hex_command = { 0 };



/*   HEX COMMAND.  */

void H_cmnd() {
    byte ch, i, len, text_row, lf_count;
    word num_output;

//h_process:
    while (true) {
        num_output = count;
        if (infinite)
            num_output = 65535;
        ch = last_hex_command;
        if (command != 'A') {
            last_hex_command =
                ch = Input_command("\x4f" RVID "Input     Output                       "
                                   RVID "                                      ");
        }
        if (ch == CONTROLC || ch == esc_code)
            return;

        if (ch == 'I' && (oa.file_disposition == view_only))
            ch = 0;

        if (ch == 'I') {            /* HEX INPUT OF A STRING */
            text_row = row;
            if (Input_line("\xb" "Hex value: ", s_hex) == CONTROLC) return;
            Move_name(input_buffer, s_hex);
            if (!Convert_string_to_hex()) {
                Error(invalid_hex);
                return;
            }
            Set_dirty();
            Save_line(text_row);    /* WILL UPDATE CURRENT LINE */
            lf_count = 0;
            for (i = 1; i <= input_buffer[0]; i++)
                if (input_buffer[i] == LF)
                    lf_count++;

            if (text_row + lf_count >= message_line)
                V_cmnd();
            else if (lf_count != 0) {
                /* ignore previous contents of the rest of screen */
                memset(&have[text_row + 1], 0, sizeof(have[0]) * (last_text_line - text_row));
            }
            memcpy(oa.low_e, &input_buffer[1], input_buffer[0]);
            oa.low_e += input_buffer[0];
        } else if (ch == 'O') {   /*    HEX OUTPUT IS HERE    */
            Rebuild_screen();
            Set_tag(ed_tagb, oa.high_s);
            cursor = oa.high_s;
            while (num_output != 0) {
                len = Min(10, num_output);
                Init_str(tmp_str, sizeof(tmp_str));
                Add_str_str("\x6" " Hex: ");
                for (i = 1; i <= len; i++, cursor++) {
                    ch = *cursor;
                    if (ch == LF) {
                        if (cursor >= oa.high_e && !Forward_file()) {
                            num_output = len; /* force termination */
                            break;
                        }
                    } else if (ch == '|') {
                        if (cursor >= oa.high_e - 1 && !Can_forward_file()) {
                            num_output = len; /* force termination */
                            break;
                        }
                    }
                    Add_str_char(' ');
                    Add_str_char(hex_digits[ch >> 4]);
                    Add_str_char(hex_digits[ch & 15]);
                }
                //        exit_loop:
                force_writing = _TRUE;
                Print_message(tmp_str);
                force_writing = _FALSE;
                num_output = num_output - len;
                if (num_output != 0) {
                    /* SEE IF USER WISHES TO CONTINUE WITH HEX LIST */
                    if (Hit_space() != ' ')
                        num_output = 0;
                }
            }
            if (macro_exec_level != 0)
                Co_flush();
            Jump_tag(ed_tagb);
        } else {
            Illegal_command();
            command = 'H'; /* doesn't allow an infinite loop if the command is 'A' */
            continue;
        }
        return;
    }
} /* h_cmnd */




/*
    I_CMND                        INSERT COMMAND
*/
void I_cmnd() {

    pointer str_p;
    byte ch, not_blank, end_col, i, k, actcol, want_slash, cb;

    Set_dirty();
    have_slash = want_slash = infinite;

    Set_tagi_at_lowe();    /* REMEMBER START IN CASE CONTROL C */
    if (want_slash) {     /* /I CAUSES LF TO BE ADDED AFTER CURSOR */
        Save_line(row);
        oa.high_s--;
        high_s_byte = LF;
        Put_insert_line(1);                /* INSERT LINE IF POSSIBLE */
    }
    Re_view();            /* FIX CURSOR IN CASE ON DEAD SPACE */

    Print_prompt_and_repos("\x9" RVID "[insert]");

    // a_begin_loop:
    while ((ch = Cmd_ci()) != esc_code) {

        if (ch != home_code)
            last_move_command = 0; /* undefined */

        Clear_message();
        count = 1;             /* NO REPEAT */
        infinite = _FALSE;

        if (ch == macro_exec_code) {
            Handle_macro_exec_code();
            continue;
        }

        /*    HANDLE MOVE TYPE COMMANDS FIRST    */

        if (ch >= up_code && ch <= home_code) {
            /* GET RID OF THE LINE FEED */
            Remove_slash();
            /* DO THE MOVE NOW */
            Move_cursor(ch);     /* A MOVE RESETS STARTING LOCATION */
            Set_tagi_at_lowe();
        }
        else if (ch == rubout_code) {
            /* HANDLE RUBOUT NEXT TO AVOID UNNECESSARY FLASHING */
            Rubout_cmnd();

            /*    HANDLE EVERYTHING ELSE    */

        /* IF IN /I AND HAVE ANYTHING BUT AN INSERT THEN REMOVE THE INSERTED LINE
        FEED. A DELETE SUBCOMMAND THEN WILL RE_VIEW TO MAKE POINTERS REFLECT
        THE ABSENCE OF THE LF. IF THIS RE_VIEW WERE NOT DONE, THE DELETE LINE
        WOULD BE A LITTLE MORE COMPLICATED BUT THERE WOULD BE A LITTLE
        LESS SCREEN ACTIVITY */

        }
        else {
            /* REMOVE EXTRA LF */
            if (Is_illegal(ch))
                Remove_slash();

            if (ch == CONTROLC) {    /* ABANDON THE INSERT */
                /* Abandon the insert if the user hit controlc */
                Delete_to_tag(ed_tagi);
                return;
            }

            /* EVERYTHING WILL CHANGE THE CURRENT ROW */
            Save_line(row);

            if (ch == n_var_code) {
                str_p = Get_n_var(radix, _TRUE);
                if (str_p[0] == 0xff) {/* controlc instead of second char */
                    /* Abandon the exchange if the user hit controlc */
                    Delete_to_tag(ed_tagi);
                    return;
                }
                for (k = 1; k <= str_p[0]; k++) {
                    if (str_p[k] == LF)
                        V_cmnd();
                    Insert_a_char(str_p[k]);
                }
            }
            else if (ch == s_var_code) {
                str_p = Get_s_var();
                if (str_p[0] == 0xff) {/* controlc instead of second char */
                    /* Abandon the exchange if the user hit controlc */
                    Delete_to_tag(ed_tagi);
                    return;
                }
                for (k = 1; k <= str_p[0]; k++) {
                    if (str_p[k] == LF)
                        V_cmnd();
                    Insert_a_char(str_p[k]);
                }
            }
            else if (ch == hexin_code) {
                byte ch, err;
                ch = Get_hexin(&err);
                if (err == 0 /* no error */) {
                    if (ch == LF)
                        V_cmnd();
                    Insert_a_char(ch);
                }
                else if (err == CONTROLC) {
                    /* Abandon the insert if the user hit controlc */
                    Delete_to_tag(ed_tagi);
                    return;
                }
            }
            else {
                if (Dispatched(ch, i_dispatch)) /* SPECIAL CASES */
                    Set_tagi_at_lowe();
                else if (Is_illegal(ch)) {
                    /* check for control-key macros (since it can't be an input ch) */
                    if (!Single_char_macro(ch))
                        AeditBeep();
                }
                else {                        /* HAVE A REAL INSERT */
                 /* PUT EXTRA LF BACK IF NECESSARY */
                    if (want_slash && have_slash == _FALSE) {
                        have_slash = _TRUE;
                        oa.high_s--;
                        high_s_byte = LF;
                        Put_insert_line(1);
                    }

                    if (ch == CR || ch == LF) {       /* INSERT CR,LF PAIR */
                        Crlf_and_indent();
                    }
                    else {
                        /*    MAY HAVE TO WRAP. TRY TO WRAP AN ENTIRE TOKEN */
                        Position_in_text();
                        actcol = virtual_col;    /* NEED FOR TAB EXPANSION */
                        end_col = actcol + 1;    /* FIND ENDING COLUMN FOR WRAP TEST */
                        if (ch == TAB) end_col = col + tab_to[actcol];
                        not_blank = _TRUE; /* TO ENSURE WRAPPING BLANKS ARE NOT INSERTED */
                        if (end_col > right_margin + 1 /* ???*/ && crlf_insert) {
                            actcol = 0;
                            cursor = oa.low_e - 1;
                            not_blank = ch != ' ' && ch != TAB;
                            if (not_blank) {
                                cb = *cursor;
                                while (cb != ' ' && cb != TAB && cb != LF)
                                    cb = *--cursor;

                                /*        DONT WIPE OUT ENTIRE LINE */
                                if (*cursor == LF)
                                    cursor = oa.low_e - 1;
                            }

                            /* CURSOR SHOULD BE TARGET FOR BACKUP */
                            if (++cursor == oa.low_e)
                                cursor = oa.high_s;

                            /* BACKUP TO PLACE WHERE CR,LF GOES    */
                            Set_tag(ed_taga, oa.high_s);
                            Re_window();

                            /* STRIP TRAILING BLANKS    */
                            oa.low_e = oa.low_e - 1;
                            while (low_e_byte == ' ' || low_e_byte == TAB)
                                oa.low_e = oa.low_e - 1;

                            oa.low_e = oa.low_e + 1;
                            Clean_tags();

                            Crlf_and_indent();
                            Jump_tag(ed_taga);
                        }
                        /* DO NOT INSERT IF TAB OR SPACE CAUSED A WRAP */
                        if (not_blank) {
                            /* IF TAB MAY HAVE TO INSERT SPACES INSTEAD    */
                            if (ch == TAB && blank_for_tab) {
                                for (i = 1; i <= tab_to[actcol]; i++) {
                                    low_e_byte = ' ';        /* INSERT BLANKS FOR TAB */
                                    oa.low_e++;
                                }
                            }
                            else {
                                low_e_byte = ch;            /* FINALLY, INSERT ONE CHAR */
                                oa.low_e++;
                            }
                        }
                    }
                }
            }
        }
        Re_view();
        last_cmd = ch;
    } /* a_begin_loop */


    /*MAY NEED TO REMOVE INSERTED LF    -  CAUSES REDUNDANT RE_VIEW */
    Remove_slash();

} /* i_cmnd */





void J_cmnd() {


    byte ch;
    boolean err;

    //  redo:
    while (1) {
        ch = Input_command("\x4f" RVID  "A_tag     B_tag     C_tag     D_tag    "
            RVID  "Start     End       Line      Position");

        command = 'J';                /* RESET FOR 'A' COMMAND */
        Init_scan(&input_buffer[1]);
        if (ch == 'S') {
            Jump_start();
        }
        else if (ch == 'E') {
            Jump_end();
        }
        else if (ch >= 'A' && ch <= 'D') {
            if (oa.tblock[ch - '@'] > oa.wk1_blocks + oa.wk2_blocks + 1) Error("\xb" "no such tag");
            else Jump_tag(ch - '@');          /* TAG A IS 1, B IS 2 ETC */
        }
        else if (ch == 'L') {        /* WANT ABSOLUTE LINE NUMBER */
            if (Input_line("\x6" "Line: ", "") == CONTROLC) return;
            input_buffer[input_buffer[0] + 1] = CR;
            count = Num_in(&err);    /* GET THE NUMBER INPUT */
            if (count != 0 && !err && Skip_char(CR)) {
                Jump_start();
                if (count > 1) Forward_line(count - 1);
            }
            else {
                Error(illegal_value);
            }
        }
        else if (ch == 'P') {    /* SET CURSOR TO SPECIFIED COLUMN */
            if (Input_line("\xa" "Position: ", "") == CONTROLC) return;
            input_buffer[input_buffer[0] + 1] = CR;
            count = Num_in(&err);    /* WANT THIS COLUMN */
            if (count <= 254 && !err && Skip_char(CR)) {
                virtual_col = (byte)count;    /* WANT THIS COLUMN */
                cursor = Backup_line(0, oa.low_e, _TRUE);
                Reset_column();
            }
            else {
                Error(illegal_value);
            }
        }
        else if (ch != CONTROLC && ch != esc_code) {
            Illegal_command();
            continue;
        }
        else {
            command = CONTROLC;    /* DO NOT REPEAT    */
        }
        return;
    }
} /* j_cmnd */





void T_cmnd() {
    byte ch;

    //redo:
    while (1) {
        ch = Input_command("\x4f" RVID "A_tag     B_tag     C_tag     D_tag    "
            RVID "                                      ");
        if (ch >= 'A' && ch <= 'D') {
            Set_tag(ch - '@', oa.high_s);          /* TAG A IS 1, B IS 2 ETC */
        }
        else {
            if (ch != CONTROLC && ch != esc_code) {
                Illegal_command();
                continue;
            }
        }
        return;
    }

} /* t_cmnd */



nestedProc void Exchange_a_char(byte ch, boolean string_input) {

    byte i;
    pointer saver;

    if (xbuf_index == xbuf_size) {
        Error("\x14" "xchange limit is 100");
    }
    else {
        if (high_s_byte != CR && high_s_byte != LF && oa.high_s + 1 < oa.high_e) {
            xbuf[xbuf_index++] = high_s_byte;
            oa.high_s++;
        }
        else
            xbuf_added++;
        if ((ch == CR || ch == LF) && !string_input)
            Crlf_and_indent();
        else {
            low_e_byte = ch;                /* INSERT ONE CHARACTER */
            oa.low_e++;
        }
        /* the byte at high_s is deleted, and the new char is inserted
           at low_e. if a tag was pointing to high_s, it must point now
           to the byte before low_e (if there is one). */
        saver = oa.toff[ed_tagw];
        for (i = 1; i <= num_tag; i++) {
            if (oa.tblock[i] == oa.wk1_blocks
                && oa.toff[i] >= oa.low_e
                && oa.toff[i] < oa.high_s) {
                if (oa.low_e > oa.low_s)
                    oa.toff[i] = oa.low_e - 1;
                else
                    oa.toff[i] = oa.high_s;
            }
        }
        if (in_other != w_in_other)
            oa.toff[ed_tagw] = saver;
    }
} /* exchange_a_char */


void X_cmnd() {

    pointer str_p;
    byte ch, j, k;
    boolean string_input;

    Set_dirty();
    Xbuf_clean();                /* REMEMBER START IN CASE CONTROL C */
    Print_prompt_and_repos("\xb" RVID "[exchange]");
    string_input = _FALSE;
    Re_view();            /* FIX CURSOR IN CASE ON DEAD SPACE */

    // a_begin_loop:
    while ((ch = Cmd_ci()) != esc_code) {

        if (ch != home_code)
            last_move_command = 0; /* undefined */

        Clear_message();
        count = 1;                                /* NO REPEAT */
        infinite = _FALSE;

        if (ch == macro_exec_code) {
            Handle_macro_exec_code();
            continue;
        }

        if (Move_cmnd(ch) || Dispatched(ch, x_reset_dispatch))
            /* A MOVE RESETS START LOC  AND CLEARS THE X BUFFER.
               A DELETE LINE AND DELETE LEFT ALSO RESETS
               STARTING X LOCATION. DELETE RIGHT DOES NOT  */
            Xbuf_clean();
        else {
            Save_line(row);                /* MAY MODIFY CURRENT LINE */
            if (ch == n_var_code) {
                str_p = Get_n_var(radix, _TRUE);
                if (str_p[0] == 0xff) {/* controlc instead of second char */
                    /* Abandon the exchange if the user hit controlc */
                    xx_controlc();
                    return;
                }
                string_input = _TRUE;
                for (k = 1; k <= str_p[0]; k++) {
                    if (str_p[k] == LF)
                        V_cmnd();
                    Exchange_a_char(str_p[k], string_input);
                }
                string_input = _FALSE;
            }
            else if (ch == s_var_code) {
                str_p = Get_s_var();
                if (str_p[0] == 0xff) {/* controlc instead of second char */
                    /* Abandon the exchange if the user hit controlc */
                    xx_controlc();
                    return;
                }
                string_input = _TRUE;
                for (k = 1; k <= str_p[0]; k++) {
                    if (str_p[k] == LF)
                        V_cmnd();
                    Exchange_a_char(str_p[k], string_input);
                }
                string_input = _FALSE;
            }
            else if (ch == hexin_code) {
                byte ch, err;
                ch = Get_hexin(&err);
                if (err == CONTROLC) {
                    /* Abandon the exchange if the user hit controlc */
                    xx_controlc();
                    return;
                }
                if (err == 0) {/* no error */
                    string_input = _TRUE;
                    if (ch == LF)
                        V_cmnd();
                    Exchange_a_char(ch, string_input);
                    string_input = _FALSE;
                }
            }
            else {
                if (Dispatched(ch, x_dispatch) == _FALSE) /* SPECIAL CASES */
                    if (Is_illegal(ch)) {
                        /* check for control-key macros (since it can't be an xch char) */
                        if (!Single_char_macro(ch))
                            AeditBeep();
                    }
                    else {
                        word rpt_count;
                        rpt_count = 1;
                        Position_in_text();
                        if (ch == TAB && blank_for_tab) {
                            rpt_count = tab_to[virtual_col]; /* NEED FOR TAB EXPANSION */
                            ch = ' ';
                        }
                        for (j = 1; j <= rpt_count; j++)
                            Exchange_a_char(ch, string_input);
                    }
                if (ch == CONTROLC) return;
            }
        }
        last_cmd = ch;
        Re_view();
    } /* a_begin_loop */
} /* x_cmnd */




/*
    V_CMND                        RE BUILD THE SCREEN
*/
void V_cmnd() {
    have[first_text_line] = 0;                    /* FLAG FOR HAVE NOTHING USEFUL ON SCREEN */
    saved_line = _FALSE;
} /* v_cmnd */
