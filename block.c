// #SMALL
// #title ('BLOCK                     THE BLOCK COMMAND')
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

// foward declarations for dispatch table
static void Block_block();
static void Block_controlc();
static void Block_delete();
static void Block_escape();
static void Block_put();

/*
    THE BLOCK_BUFFER IS USED BY THE BLOCK COMMAND TO PUT TEXT ASIDE.
    IN_BLOCK_BUFFER IS EITHER THE NUMBER OF CHARACTERS IN BLOCK_BUFFER
    OR -1 IF BLOCK_BUFFER CONTAINS THE CONTENTS OF THE LOGICAL BUFFER.
*/

#define block_buffer_size	2048

byte block_buffer[block_buffer_size];
byte block_char;
byte block_col;        /* COLUMN OF START OF BLOCK COMMAND */
byte b_is_done;        /* SET TRUE AT END OG BLOCK COMMAND */
dispatch_t b_dispatch[11] = {
    CR, &Cr_cmnd,
    esc_code, &Block_escape,
    CONTROLC, &Block_controlc,
    '-', &Fr_cmnd,
    'A', &Fr_cmnd,
    'B', &Block_block,
    'D', &Block_delete,
    'F', &Fr_cmnd,
    'J', &J_cmnd,
    'P', &Block_put,
    0, NULL };

word in_block_buffer = 0;



/* IF POSSIBLE, SAVE THE TEXT BETWEEN THE CURRENT CURSOR AND TAGI
   IN THE BLOCK BUFFER. IF CAN DO, RETURN TRUE. */

static byte Saved_in_block() {
    word len;

    if (oa.tblock[ed_tagi] == oa.wk1_blocks) {    /* TAG MUST BE IN CURRENT
                                                    BLOCK */
        if (oa.toff[ed_tagi].bp >= oa.high_s) {    /* TAG IS IN HIGH PART */
            len = (word)(oa.toff[ed_tagi].bp - oa.high_s);
            if (len <= block_buffer_size) {
                in_block_buffer = len;
                memcpy(block_buffer, oa.high_s, len); /* MOVE TEXT
                                                                TO BUFFER */
                return _TRUE;
            }
        }
        else {                                /* TAG IS IN LOW PART
                                                    OF TEXT */
            len = (word)(oa.low_e - oa.toff[ed_tagi].bp);
            if (len <= block_buffer_size) {
                in_block_buffer = len;
                memcpy(block_buffer, oa.toff[ed_tagi].bp, len);
                /* MOVE TEXT TO BUFFER */
                return _TRUE;
            }
        }
    }
    return _FALSE;
} /* saved_in_block */




/* DELETE A RANGE FOR THE BLOCK COMMAND. */

static void Block_delete() {
    byte ch, delete_down;

    /*    ON A DELETE, IF POSSIBLE PUT STUFF INTO BLOCK_BUFFER. OTHERWISE
        MUST PROMPT FOR PERMISSION TO SAVE TO BLOCK FILE */

    Set_dirty();

    /*    MUST REMEMBER IF DELETING DOWN SO CLEANUP CAN FIND THE STARTING @    */

    delete_down = oa.tblock[ed_tagi] == oa.wk1_blocks && oa.toff[ed_tagi].bp < oa.high_s;

    if (Saved_in_block() == _FALSE) {
        Rebuild_screen();      /* BRING BACK DISPLAY IF SUPPRESSED */
        ch = Input_yes_no_from_console("cannot save in memory - save anyway?", _TRUE, _FALSE);
        if (Have_controlc()) {
            Jump_tagi();
            b_is_done = _TRUE;
            return;
        }
        if (ch) {
            in_block_buffer = 0xffff;    /* FLAG TEXT IN BLOCK FILE */
/*p            CALL set_tag(ed_taga,oa.high_s);*/
            Write_block_file(_TRUE);
            V_cmnd();
            /*p            CALL jump_tag(ed_taga);*/
        }
    }

    Delete_to_tag(ed_tagi);
    if (delete_down)
        oa.toff[ed_tagi].bp = oa.low_e;
    b_is_done = _TRUE;
    return;
} /* block_delete */




/* THE USER HAS DEFINED A NEW BLOCK. PUT IN BLOCK BUFFER IF POSSIBLE,
   OTHERWISE WRITE TO BLOCK FILE. */

static void Block_block() {

    if (Saved_in_block() == _FALSE) {
        in_block_buffer = 0xffff;    /* FLAG FOR TEXT IN BLOCK FILE */
        Write_block_file(_FALSE);
    }
    b_is_done = _TRUE;
    return;
} /* block_block */




/* PUT THE BLOCK TO A FILE. */

static void Block_put() {

    //redo:
    while (1) {
        if (Input_filename("\xd" "Output file: ", s_put_file) == CONTROLC)
            Jump_tagi();
        else if (input_buffer[0] == 0)
            Error(invalid_null_name);
        else if (Test_file_existence(util_file))
            continue;
        else
            Write_util_file();
        break;
    }
    b_is_done = _TRUE;
} /* block_put */




/* DO NOTHING. JUST QUIT. */

static void Block_escape() {
    b_is_done = _TRUE;
} /* block_escape */




/* GO BACK TO START AND QUIT. */

static void Block_controlc() {
    Jump_tagi();
    b_is_done = _TRUE;
} /* Block_controlc */






boolean first_at_sign_is_here;
/* prevents from writing extra chars if we don't need them */


static void Draw_first_at_sign(byte ch) {
    byte i;

    /* IF ON SCREEN, FIND THE ROW CONTAINING THE STARTING @    */
    if (oa.tblock[ed_tagi] == oa.wk1_blocks          &&
        oa.toff[ed_tagi].bp >= have[first_text_line] &&
        oa.toff[ed_tagi].bp < have[message_line]) {
        if (!first_at_sign_is_here) {
            for (i = first_text_line + 1; have[i] <= oa.toff[ed_tagi].bp; i++)
                ;
            Put_goto(block_col, i - 1);
            Text_co(ch);
            first_at_sign_is_here = _TRUE;
        }
    }
    else
        first_at_sign_is_here = _FALSE;
} /* draw_first_at_sign */



/* REMOVE THE '@' OR WHATEVER THAT MARKS THE STARTING POINT OF THE BLOCK.
   ALSO DELETE TAGI. */

static void B_cleanup() {

    first_at_sign_is_here = _FALSE; /* force drawing */
    Draw_first_at_sign(block_char);

} /* b_cleanup */





/* RETURNS CHARACTER UNDER CURSOR ON SCREEN */

static byte On_screen() {
    byte ch;
    pointer last_ptr;

    ch = Printable(high_s_byte);
    if (ch == TAB || ch == LF)
        ch = ' ';
    /* IF THE CHARACTER IS ON DEAD SPACE THEN IT PRINTS AS A BLANK */
    last_ptr = oa.low_e - 1;
    if (col >= line_size[row] || (*last_ptr == TAB && tab_to[oa.left_column + col - 1] != 1))
        ch = print_as[' '];

    /* IF BEFORE LEFTCOL or after rightcol THEN PRINTS AS ! */
    if ((col == 0 && oa.left_column > 0) || (col == 79 && line_size[row] > 79))
        ch = '!';
    return ch;
} /* on_screen */




/*  BLOCK COMMAND  */

void B_cmnd() {

    byte t_col, t_row;

    Position_in_text();
    block_col = col;                /* SAVE COLUMN AND CURRENT CHARACTER SO
                                    THE '@' MARKER CAN BE REMOVED EVENTUALLY */
    block_char = On_screen();
    Text_co('@');
    first_at_sign_is_here = _TRUE;
    Set_tag(ed_tagi, oa.high_s);        /* TAGI SAVES STARTING LOCATION */
    b_is_done = _FALSE;                    /* DIRTY FLAG TO SAY WHEN DONE */

    command = 0; /* don't allow an AGAIN as a first command. */
    while (_TRUE) {
        if (command != 'A') last_cmd = command;
        command = Input_command("\x4f"
            RVID "Buffer    Delete    Find      -find    "
            RVID "Jump      Put                         ");
        if (command == 'A' && last_cmd != 'F' && last_cmd != '-')
            command = last_cmd;
        Text_co(On_screen());    /* REMOVE CURRENT '@' */
        if (oa.high_s == oa.toff[ed_tagi].bp && col == block_col)
            /* mark again with @ if we are at the first @ (tagi)
               and not on dead_blank */
            Text_co('@');

        if (Move_cmnd(command) == _FALSE) {        /* DO A MOVE */

            if ((command == 'D') && oa.file_disposition == view_only)
                command = 0; /* illegal command */

            if (Dispatched(command, b_dispatch) == _FALSE) {
                if (command == macro_exec_code)
                    Handle_macro_exec_code();
                else if (!Single_char_macro(command))
                    Illegal_command();
            }

            if (b_is_done || cc_flag) {
                B_cleanup();
                return;
            }
        }
        Re_view();
        t_col = col; t_row = row;
        Draw_first_at_sign('@');
        Put_goto(t_col, t_row);
        Text_co('@');                /* MARK CURRENT LOCATION */
    }
} /* b_cmnd */





/*
    G_CMND                        GET. IF NULL STRING INPUT, COPY THE BLOCK
                                BUFFER TO THE CURRENT LOCATION. OTHERWISE READ
                                THE NAMED FILE INTO THE CURRENT LOCATION.
*/
void G_cmnd() {
    word was_saved_from;
    byte was_row;
    byte i;
    address_t nxt_addr;      // ?? should i be a word
    pointer save_addr;

    if (infinite) return;
    save_addr = have[row];
    Save_line(row);
    was_row = row;

    if (Input_filename("\xc" "Input file: ", s_get_file) == CONTROLC)
        return;
    Set_dirty();
    if (input_buffer[0] == 0 && in_block_buffer != 0xffff) {
        /*    HANDLE THE IN MEMORY BLOCK BUFFER CASE FIRST */
        i = 0xff /*ff*/;
        nxt_addr.bp = block_buffer;
        do {
            nxt_addr.bp = Next_line(nxt_addr.bp);
            i++;
        } while (nxt_addr.bp <= block_buffer + in_block_buffer);

        i *= count;
        if (i < last_text_line - was_row && count < 23 && i < 23) {
            nxt_addr.w = was_row;
            if (cursor == save_addr && was_row > 0) {
                nxt_addr.w = was_row - 1;
                Save_line(was_row - 1);
                have[was_row] = save_addr;
            }
            Put_start_row((byte)nxt_addr.w);
            Put_insert_line(i);
        }

        while (count > 0 && cc_flag == _FALSE) {
            Check_window(in_block_buffer);
            oa.high_s = oa.high_s - in_block_buffer;
            memcpy(oa.high_s, block_buffer, in_block_buffer);
            count--;
        }

    }
    else {
        /*    OTHERWISE MUST READ A FILE INTO TEXT    */
        /* MUST REMEMBER DISTANCE BETWEEN SAVED_FROM (STARTING POINT OF CURRENT
           LINE) AND LOW_E AS THE RE-WINDOW AT BEGINNING OF STUFF READ CAUSES
           SAVED_FROM AND HAVE(CURRENT LINE) TO POINT TO THE WRONG LOCATION */
        was_saved_from = (word)(oa.low_e - saved_from);

        if (input_buffer[0] == 0)
            Read_block_file();
        else
            Read_util_file();

        if (have[first_text_line] != 0) {
            if (was_saved_from == 0)
                saved_from = have[was_row] = oa.high_s;
            else
                saved_from = have[was_row] = oa.low_e - was_saved_from;
        }
    }
} /* g_cmnd */


