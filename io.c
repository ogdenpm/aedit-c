// #SMALL
// #title('IO               INPUT OUTPUT ROUTINES')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

/*  THIS MODULE CONTAINS ALL OF THE UDI ROUTINES AND ALSO IS
    RESPONSIBLE FOR ALL I/O. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef MSDOS
#include <io.h>
#endif
#include "oscompat.h"
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"



boolean poll_mode;

/*
    HANDLING OF LARGE FILES

    DURING EDITING OF LARGE FILES, THE USERS FILE IS DISTRIBUTED AS FOLLOWS

    <- WK1 -><- IN MEMORY -><- WK2 -><- STILL IN INPUT FILE ->

    THE FOLLOWING VARIABLES KEEP TRACK OF THE DISTRIBUTION

    BLOCKSIZE  WORD                        SIZE OF A BLOCK.
    WK1_BLOCKS WORD                        NUMBER OF BLOCKS OF DATA IN
                                        THE FIRST WORK FILE. A BLOCK
                                        IS BLKSIZE BYTES. THE FIRST
                                        BLOCK IS THE ZERO BLOCK
    WK2_BLOCKS WORD                        NUMBER OF BLOCKS IN WK2.
    MORE_INPUT BYTE                        TRUE IF STILL MORE TEXT IN THE
                                        ORIGINAL INPUT FILE
*/


/* THE BLOCK_ VARIABLES COMBINE TO FORM A FOUR BYTE INTERER WITH THE
   BYTES IN THE BLOCK FILE    */
word block_low = 0, block_high = 0;

#define input_mode	1
#define output_mode	2
#define update_mode	3


boolean block_put_file_attached;

static void Flush();


/*    UP TO 255 BYTES ARE BUFFERED IN CO_BUFFER. CO ADDS THE BYTES AND
    CO_FLUSH WRITES THE BUFFER    */



    /* WRITE THE CO BUFFER TO CO. IF EMPTY, JUST RETURN. */

void Co_flush() {

    Check_for_keys();
    Flush();

} /* co_flush */





static void Flush() {
    wbyte i;
    byte index, len;

    if (co_buffer[0] == 0)
        return;

    if (config == SIIIE) {
        Send_block(&co_buffer[0]);
    }
    else if (config == SIIIEO) {
        for (i = 1; i <= co_buffer[0]; i++) {
            Port_co(co_buffer[i]);
        }
    }
    else {

        if (delay_after_each_char != 0) {
            if (delay_after_each_char <= 0x8000) {
                for (i = 1; i <= co_buffer[0]; i++) {
                    co_write(&co_buffer[i], 1);
                    if (delay_after_each_char != 0xffff)  /* ffff - no delay */
                        ms_sleep(delay_after_each_char / 10);
                }
            }
            else {
                index = 1;
                len = Min(delay_after_each_char - 0x8000, co_buffer[0] - index + 1);
                while (len != 0) {
                    co_write(&co_buffer[index], len);
                    ms_sleep(1);
                    index = index + len;
                    len = Min(delay_after_each_char - 0x8000, co_buffer[0] - index + 1);
                }
            }
        }
        else {
            co_write(&co_buffer[1], co_buffer[0]);
        }

    }
    co_buffer[0] = 0;

} /* flush */






/* CLASSIC :CO: ROUTINE EXCEPT THAT IS BUFFERED.
   only PUT_INDICATOR calls CO directly, because it is not
   called within macros or in batch_mode. the others
   call PRINT_STRING and PRINT_UPDATE. */

void Co(byte byt) {

    if (co_buffer[0] == last(co_buffer))
        Co_flush();
    co_buffer[++co_buffer[0]] = byt;

} /* co */




void Print_unconditionally(char *s) {
    while (*s)
        Co(*s++);
} /* print_unconditionally */




void Print_unconditionally_p(pointer string) {  // original version
    for (int len = *string; len > 0; len--)
        Co(*++string);

} /* print_unconditionally */



void Print_unconditionally_crlf() {
#ifdef MSDOS
    Print_unconditionally("\r\n");
#else
    Print_unconditionally("\n");

#endif
} /* print_unconditionally_crlf */







/* OUTPUT A STRING. USED TO PRINT CONTROL SEQUENCES AND LINES. */

void Print_string(pointer string) {

    if (batch_mode)
        return;
    if (macro_suppress && !force_writing && macro_exec_level != 0)
        return;
    Print_unconditionally_p(string);
} /* print_string */





/*                      OUTPUT A STRING. USED TO PRINT CHANGES IN A LINE
                        FOUND BY REBUILD. SIMILAR TO PRINT_STRING BUT MUST
                        CHECK FOR THE REVERSE VIDEO LEADIN (AFRV) AND
                        EXPAND IT.
                        This routine also contains the smart for putting
                        out the Normal Video Leading (AFNV) at the end of
                        a line (position 79) if it is not the last line
                        (is a window prompt line) or is invisible
*/

void Print_update(byte cur_column, byte len, pointer string, boolean rev_vid) {
    byte max, y_save, r_found, x_save;

    x_save = len;
    r_found = _FALSE;
    col = cur_column;

    if (macro_suppress && !force_writing && macro_exec_level != 0)
        return;
    max = prompt_line;
    if (window.prompt_line > max)
        max = window.prompt_line;

    if (*string == rvid) {
        if (!character_attributes && output_codes[normal_video_code].code[0] > 0
            && (row != max || ~visible_attributes)) {
            y_save = row;
            Put_normal_video_end();
            Put_goto(cur_column, y_save);
        }
    }

    if (rev_vid && character_attributes && *string != rvid) {
        if (output_codes[normal_video_code].code[0] > 0)
            r_found = _TRUE;
        if (output_codes[reverse_video_code].code[0] > 0)
            Put_reverse_video();
    }

    while (len > 0) {
        if (*string == rvid) {
            if (output_codes[normal_video_code].code[0] > 0)
                r_found = _TRUE;
            if (output_codes[reverse_video_code].code[0] > 0) {
                if (!visible_attributes) Co(' ');
                Put_reverse_video();
            }
            else {
                Co(' ');
            }
        }
        else {
            Co(*string);
        }
        string++;
        len--;
    }
    col = cur_column + x_save;

    if (r_found && character_attributes &&
        (row != max || ~visible_attributes)) {
        y_save = row;
        Put_normal_video_end();
        Put_goto(cur_column + x_save, y_save);
    }

} /* print_update */






byte ci_buff[256];
byte cur_ci = { 0 };
byte valid_ci = { 0 };



byte cc_code_buf[5] = { 1, CONTROLC, 0, 0, 0 };/** RR Jan 16 **/

/*******************************************************
   Used to allow configuring controlc.
*******************************************************/
static byte Convert_if_cc(byte _char) {                         /** RR Jan 16 **/
   /* this mini PROC allows configurable controlc */
    if (_char == cc_code_buf[1])
        return CONTROLC;
    return _char;
} /* convert_if_cc */



/*******************************************************
   abort rest of input buffer on a control-c
   and leave one control-c in the buffer.
*******************************************************/
void Handle_controlc() {
    cur_ci = 0;
    valid_ci = 1;
    ci_buff[0] = CONTROLC;
    cc_flag = _TRUE;
    quit_state = 0;
} /* handle_controlc */





/************************************************************
    Called  now and then primarily to check for controlc, but
    if a key is read  in,  it  is  put  into  ci$buff.
    controlc aborts the  type-ahead buffer and sets
    cc_flag to  true.  some  pre-parsing is done  here, so we
    can   stop  reading   in  keys   when  we   are  exiting.
    Quit$state=0 - not in quit command, or I has been pressed
    or not at main command  level;  quit$state=1  -  in  quit
    command;  quit$state=2 - in W sub command; quit$state=3 -
    output  shut$off (A or E encounter when in quit$state 1);
    quit$state=FFH - At main command level.
************************************************************/
void Check_for_keys() {
    byte ch, _char, numin, nx_ch;

    if (batch_mode == _TRUE)
        return;

    if (in_block_mode == _TRUE) {
        Codat(0xff);
        in_block_mode = _FALSE;
    }

    if (submit_mode == _TRUE)
        return;

    if (do_type_ahead != _TRUE /* !!! */) {
        /* !!! must remain "<>TRUE" because there are three
               valuse: TRUE,FALSE and undefined. The value
               is undefined during initialization to prevent
               using dq$special 3 before processing the macro
               file, because we dont know yet the setting of AT */
        if (have_control_c) {
            Handle_controlc();
            have_control_c = _FALSE;
        }
        return;
    }

    if (config != SIIIE && config != SIIIEO) {
        if (!poll_mode) {
            set_ci_mode(3); // =set to polling
            poll_mode = _TRUE;
        }
    }

    //    get_next_character:
    while (1) {
        if (quit_state == 3)
            return;    /* read in nothing */

        numin = 0;

        if (config == SIIIE || config == SIIIEO) {
            if (Port_csts()) {
                _char = Port_ci();
                numin = 1;
            }
        }
        else {
            if (have_control_c) {
                _char = CONTROLC;
                numin = 1;
                have_control_c = _FALSE;
            }
            else {
                _char = 0;
                numin = (byte)ci_read(&_char);
            }
        }
        if (numin == 0)
            return;

        _char = Convert_if_cc(_char); /** RR Jan 16 **/

        nx_ch = valid_ci + 1;

        if (_char == CONTROLC) {
            Handle_controlc();
            return;
        }

        if (nx_ch == cur_ci)
            return;    /* buffer would wrap, so don't store */
        ch = Upper(_char);
        if (quit_state == 0xff) {
            if (ch == 'Q')
                quit_state = 1;
        }
        else if (quit_state == 1) {
            if (ch == 'E' || ch == 'A') {
                quit_state = 3;
            }
            else if (ch == 'I') {
                quit_state = 0;
            }
            else if (ch == 'W') {
                quit_state = 2;
            }
            else if (_char == input_codes[esc_code - first_code].code[1]) {
                quit_state = 0;
            }
        }
        else if (quit_state == 2) {
            if (_char == input_codes[esc_code - first_code].code[1] || _char == CR)
                quit_state = 1;
        }
        ci_buff[valid_ci] = _char;
        valid_ci++;
    }
    /* as long as we are here, see if other chars can be read in */

} /* check_for_keys */





  /* SET_CURSOR - set the cursor using the parameter byte specified */


#define cursor_type_offset	0x75

byte set_string[] = { 4, 0x1b, 'X', cursor_type_offset, 0 };

void Set_cursor(byte new_val) {

#define flag_start	0x5a80


    if (config != SIIIE)
        return;    /* protection */
    set_string[4] = new_val;
    Send_block(set_string);

} /* set_cursor */








/****************************************************************************
    The following  three PROCs  are used  to indicate  the state  of the
editor.  When  the editor waits for  input from the keyboard  (i.e.  the type
ahead  buffer is empty), the  third and fourth chars  in the message-line are
converted to '?'.  When a key is typed, these _char are converted back to '!'.
****************************************************************************/

boolean input_expected_flag;    /* initial(true) gets its value in  alter_init */
byte current_indicator = '-';



/*************************************************/
/* if ch='!' set the option. if ch='-' reset it. */
/*************************************************/
void Set_input_expected(byte ch) {

    if (batch_mode)
        ch = '-';
    next_message[3] = next_message[4] = ch;
    /* spoil current message to force rewriting */
    current_message[3] = current_message[4] = 'X';
    input_expected_flag = (ch == '!');
    current_indicator = ch;

} /* set_input_expected */









static void Put_indicator(byte ch) {

    byte old_row, old_col;
    boolean save_force_writing;
    boolean in_block_save;

    if (macro_exec_level != 0 || batch_mode)
        return;
    if (!input_expected_flag)
        return;
    if (current_indicator == ch)
        return;
    current_indicator = ch;
    if (error_status.in_invocation)
        return;

    in_block_save = in_block_mode;
    if (in_block_mode == _TRUE) {
        /* cursor addressing is meaningless in block mode.
           close block mode temporarily. */
        Flush();
        Codat(0xff);
        in_block_mode = _FALSE;
    }
    save_force_writing = force_writing;
    force_writing = _TRUE;
    old_col = col;
    old_row = row;
    Put_goto(2, message_line);
    Co(ch);
    Co(ch);
    col = col + 2;     /* adding 2 because two chars will be written */
    Put_goto(old_col, old_row);
    Flush(); /* don't check$for$key because we are
                   at the middle of reading a _char */
    next_message[3] = next_message[4] = ch;
    force_writing = save_force_writing;

    if (in_block_save == _TRUE) {
        /* restore previous mode */
        Cocom(0x2f);
        in_block_mode = _TRUE;
    }

} /* put_indicator */



static void Input_expected() {
    Put_indicator('?');
} /* input_expected */

void Working() {
    Put_indicator('!');
} /* working */





byte Ci() {
    byte _char, numin;

    Co_flush();                        /* CLEAR OUTPUT BUFFER */
//re_read:
    while (_TRUE) {
        if (cur_ci == valid_ci) {        /* type-ahead buffer empty */
            Input_expected(); /* print "input-expected" (-??-)  message */
            if (config != SIIIE && config != SIIIEO) {
                if (poll_mode && batch_mode == _FALSE) {
                    set_ci_mode(1);     // transparent non polling
                    poll_mode = _FALSE;
                }
            }

            _char = 0;

            if (config == SIIIE || config == SIIIEO) {
                if (config == SIIIE) {
                    Set_cursor(cursor_type);    /* restore user's cursor */
                }
                _char = Port_ci();
                numin = 1;
            }
            else {
                numin = (byte)ci_read(&_char);
            }
            _char = Convert_if_cc(_char); /** RR Jan 16 **/
            if (have_control_c) {
                _char = CONTROLC;
                numin = 1;
                have_control_c = _FALSE;
            }
#ifdef MSDOS
            /* SERIES IV CAN RETURN A SPURIOUS ZERO LENGTH RESPONSE
                IF CONTROL D OR BREAK IS HIT */
            if (numin == 0)
                continue;
#else
            if (numin == 0) {
                Error("\xd" "premature EOF");
                Quit_exit(5);
            }
#endif
        }
        else {    /* get a _char from the buffer */
            _char = ci_buff[cur_ci];
            cur_ci++;
            numin = 1;
        }

        if (_char == CONTROLC)
            cc_flag = _TRUE;

        /* MUST TEST FOR READING A CONTROLC BECAUSE OF AN ISIS BUG
           ASSOCIATED WITH A CONSOLE REDIRECTED TO :I1:   --old comment-- */

        if (config == SIIIE)
            Set_cursor(0x40);    /* turn cursor off */
        return _char;
}
} /* ci */





/*  PRINT A UDI ERROR MESSAGE IN MESSAGE IF EXCEP <> 0  */

void Echeck() {
    byte err_buf[81];
    word eexcep;

    if (excep != 0) {
        eexcep = excep;                    /* SAVE THE EXCEPTION CODE */
        Init_str(tmp_str, sizeof(tmp_str));
        Add_str_special(files[file_num].name);
        /* Only have room for first 20 chars of file name */
        if (tmp_str[0] > 20)
            tmp_str[0] = 20;
        Add_str_char(' ');
        dq_decode_exception(excep, err_buf, 60);
        Add_str_str(err_buf);
        Error(tmp_str);
        excep = eexcep;
    }
} /* echeck */



/*  PRINT A UDI ERROR MESSAGE IN MESSAGE IF EXCEP <> 0  */

void Echeck_no_file() {
    byte err_buf[81];

    if (excep != 0) {
        dq_decode_exception(excep, err_buf, 80);
        Error(err_buf);
    }
} /* echeck_no_file */




/*
    OPENI                            OPEN A FILE IN INPUT MODE
                                    DO NOT CHECK STATUS AS MAY BE NEW FILE
*/

void Openi(byte fnum, byte nbuf) {
    char fname[FILENAME_MAX];

    toCstr(fname, files[fnum].name);
    file_num = fnum;
    excep = 0;
    if ((files[fnum].conn = fopen(fname, "rb")) == NULL)
        excep = errno;
} /* openi */




/*
    OPEN_BLOCK                            OPEN THE ALREADY ATTACHED BLOCK FILE.
*/

static void Open_block(byte mode) {     // attached not used so this isn't called

    Working();

    file_num = block_file;
} /* open_block */



/*
    TEST_FILE_EXISTENCE                    Test if the file exists, returns
                                        True if exists and user doesn't
                                        want to overwrite it
*/



byte Test_file_existence(byte fnum) {
    byte ans;
    char fname[FILENAME_MAX];
    toCstr(fname, files[fnum].name);

    Working();
    if (!*fname || access(fname, 0) != 0) {     // null file or doesn't exist
        excep = 0;
        return _FALSE;
    }

    Rebuild_screen();
    if ((ans = Input_yes_no_from_console("\x18" "overwrite existing file?", _FALSE, _FALSE)) == CONTROLC)
        return _TRUE;
    return ~ans;
} /* test_file_existence */





/*
    OPENO    OPEN A FILE IN OUTPUT OR UPDATE MODE
*/

void Openo(byte fnum, byte mode, byte nbuf) {
    char fname[FILENAME_MAX];

    toCstr(fname, files[fnum].name);
    file_num = fnum;
    excep = 0;
    if ((files[fnum].conn = fopen(fname, mode == update_mode ? "wb+" : "wb")) == NULL) {
        excep = errno;
        Echeck();
    }
} /* openo */




/*
    OPEN_CICO                    OPEN :CI: AND :CO:
*/
void Open_cico() {
    files[co_file].conn = stdout;   // :CO: already open
    files[ci_file].conn = stdin;    // :CI: already open
    set_ci_mode(1);                 // transparent non polling
    submit_mode = _FALSE;
    poll_mode = _FALSE;
} /* open_cico */




/*
    DETACH        DETACH THE FILE
*/
void Detach(byte fnum) {

    Working();

    file_num = fnum;
    excep = fclose(files[fnum].conn) == 0 ? 0 : errno;
    Echeck();
    files[fnum].conn = NULL;
} /* detach */




/*
    CLOSE_BLOCK                    CLOSE BUT DO NOT DETACH THE BLOCK FILE.
*/

static void Close_block() {
    file_num = block_file;
    excep = fclose(files[block_file].conn) == 0 ? 0 : errno;
    files[block_file].conn = NULL;
    Echeck();
} /* close_block */




/*
    DETACH_INPUT                DETACH INPUT FILE AND FLAG AS CLOSED
*/
void Detach_input() {

    Working();
    /*
     * Begin R2 Fix, Part 1, PPR: 2984, a problem with multi-access qe/qw...,
     *               2/2/87
     */
     /*    IF oa.file_disposition<>view_only
             THEN CALL detach(in_file);
     */
     /*
      * End R2 Fix, Part 1, PPR: 2984
      */
    oa.more_input = _FALSE;
} /* detach_input */




/*
    READ    DO A DQ$READ        ALWAYS READ INTO LOW_E
*/
word Read(byte fnum) {

    word numin;

    Working();

    file_num = fnum;
    if (config != SIIIE)
        Check_for_keys();
    numin = (word)fread(oa.low_e, 1, oa.block_size, files[fnum].conn);
    if (excep = ferror(files[fnum].conn) ? errno : 0) {
        Echeck();
        numin = 0;
    }
    if (config != SIIIE)
        Check_for_keys();
    oa.low_e = oa.low_e + numin;
    return numin;
} /* read */




/*
    REWIND        SEEK TO START OF FILE
*/
static void Rewind(byte fnum) {

    Working();

    file_num = fnum;
    rewind(files[fnum].conn);
    excep = errno; /* SEEK TO START */
    Echeck();
} /* rewind */




/*
    WRITE_WK1                            WRITE A BLOCK TO WK1. THE BLOCK IS
                                        WRITTEN FROM LOW_S.
*/

static void Write_wk1() {
    if (config != SIIIE)
        Check_for_keys();
    Write_temp(oa.wk1_conn, oa.low_s);
    if (config != SIIIE)
        Check_for_keys();
} /* write_wk1 */




/*
    WRITE_WK2                            WRITE A BLOCK TO WK2. THE BLOCK IS
                                        WRITTEN FROM HIGH_S.
*/
static void Write_wk2(pointer start) {

    if (config != SIIIE)
        Check_for_keys();
    Write_temp(oa.wk2_conn, start);
    if (config != SIIIE)
        Check_for_keys();
} /* write_wk2 */




/*
    TEXT_LOST                            PRINT 'some text lost' MESSAGE.
*/

static void Text_lost() {
    if (oa.file_disposition == lose_file)
        Error("\xe" "some text lost");
    oa.file_disposition = lost_file;
} /* text_lost */





/*
    EXPAND_WINDOW                        THE WINDOW WILL BE MADE LARGER
                                        BY WRITING TEXT TO EITHER WK1 OR WK2.
                                        TEXT IS WRITTEN FROM THE LARGER OF
                                        THE LOW PORTION AND THE HIGH PORTION.
                                        THE LOCATION OF THE WINDOW IS
                                        UNCHANGED.
*/

static void Expand_window() {
    wbyte i;
    word saver2;
    pointer limit, saver;

    saver = oa.toff[ed_tagw];
    saver2 = oa.tblock[ed_tagw];
    if ((oa.low_e - oa.low_s) > (oa.high_e - oa.high_s)) {    /* CHOOSE LARGER */

        limit = oa.low_s + oa.block_size;

        /* KILL PRINT POINTERS IF WE HAVE WRITTEN OUT PART OF SCREEN */
        if (have[first_text_line] <= limit)
            V_cmnd();

        /*    THE FILE_DISPOSITION SWITCH SAYS WHETHER ON NOT TO SAVE SPILLED TEXT */

        if ((oa.file_disposition == lose_file) ||
            (oa.file_disposition == lost_file)) {

            /*        SIMPLY THROW AWAY SOME TEXT        */
            Text_lost();
            for (i = 1; i <= num_tag; i++) {            /* FIX ANY TAGS THAT WERE THROWN AWAY*/
                if (oa.tblock[i] == oa.wk1_blocks)
                    if (oa.toff[i] < limit)
                        oa.toff[i] = limit;
            }
            /* if we just spilled the file, then the other window might not
               have valid data on it, so mark it dirty
            */
            /*             IF w_in_other=in_other THEN w_dirty=TRUE;  */

            /*        ELSE WRITE A BLOCK TO WK1_FILE    */
        }
        else {

            Write_wk1();                    /* DOES THE WRITE */

            for (i = 1; i <= num_tag; i++) {                /* FIX ANY TAGS THAT WERE PAGED OUT */
                if (oa.tblock[i] == oa.wk1_blocks) {
                    if (oa.toff[i] < limit)
                        oa.toff[i] = (pointer)(oa.toff[i] -oa.low_s);
                    else
                        oa.tblock[i] = oa.tblock[i] + 1;
                }
                else if (oa.tblock[i] > oa.wk1_blocks && oa.tblock[i] != 0xffff)
                    oa.tblock[i] = oa.tblock[i] + 1;
            }
            oa.wk1_blocks = oa.wk1_blocks + 1;
        }

        /*    NOW RECOVER THE LOW SPACE. NOTICE THAT ONE MORE BYTE THAN
            NECESSARY IS MOVED - THIS IS TO PRESERVE TAGS AT OA.LOW_E.     */

        Movmem(limit, oa.low_s, (word)(oa.low_e - limit + 1));

        oa.low_e = oa.low_e - oa.block_size;

    }
    else {                            /* NEED TO BACKUP */
        Subtract_eof();
        limit = oa.high_e - oa.block_size;            /* FIRST BYTE TO WRITE */
        /* KILL PRINT POINTERS IF WE HAVE WRITTEN OUT PART OF SCREEN */
        if (have[message_line] >= limit)
            V_cmnd();


        /*    SEE IF USER WANTS TO RETAIN SPILLED TEXT    */

        if ((oa.file_disposition == lose_file) ||
            (oa.file_disposition == lost_file)) {

            Text_lost();

            /*    FIX TAGS THAT WILL BE DISCARDED    */
            for (i = 1; i <= num_tag; i++) {
                if (oa.toff[i] >= limit)
                    oa.toff[i] = limit - 1;
            }
            /* if we just spilled the file, then the other window might not
               have valid data on it, so mark it dirty
            */
            /*             IF w_in_other=in_other THEN w_dirty=TRUE;  */

        }
        else {

            Write_wk2(limit);
            for (i = 1; i <= num_tag; i++) {                /* FIX ANY TAGS THAT WERE PAGED OUT */
                if (oa.tblock[i] > oa.wk1_blocks && oa.tblock[i] != 0xffff)
                    oa.tblock[i] = oa.tblock[i] + 1;
                else if (oa.tblock[i] == oa.wk1_blocks)
                    if (oa.toff[i] >= limit) {
                        oa.toff[i] = (pointer)(oa.toff[i] -limit);
                        oa.tblock[i] = oa.tblock[i] + 1;
                    }
            }
            oa.wk2_blocks = oa.wk2_blocks + 1;
        }

        /*    MOVE MEMORY UP TO RECLAIM SPACE    */

        Movmem(oa.high_s, oa.high_s + oa.block_size, (word)(limit - oa.high_s));
        oa.high_s = oa.high_s + oa.block_size;
        if (oa.wk2_blocks == 0 && oa.more_input == _FALSE) Add_eof();
    }

    if (in_other != w_in_other) {
        oa.toff[ed_tagw] = saver;
        oa.tblock[ed_tagw] = saver2;
    }
    else {
        w_dirty = _TRUE;
    }

} /* expand_window */




/*
    CHECK_WINDOW                    IF LESS THAN WINDOW_MINIMUM OF FREE SPACE,
                                    EXPAND THE WINDOW. THE ARGUMENT IS THE
                                    AMOUNT OF EXTRA SPACE REQUIRED.
*/
void Check_window(word plusplus) {
    while ((word)(oa.high_s - oa.low_e) < (word)(window_minimum + plusplus)) {
        Expand_window();
    }
} /* check_window */




/*
    MACRO_FILE_READ            DO A DQ$READ OF THE MACRO FILE
*/

word Macro_file_read() {

    word numin;

    Working();

    file_num = mac_file;
    Check_window(0);
    if (config != SIIIE)
        Check_for_keys();
    numin = (byte)fread(oa.low_e, 1, window_minimum, files[mac_file].conn);
    excep = ferror(files[mac_file].conn) ? errno : 0;
    Echeck();
    if (config != SIIIE)
        Check_for_keys();
    return numin;

} /* macro_file_read */




/*
    CAN_BACKUP_FILE                    TRUE IF THERE IS MORE INPUT ON DISK
*/
byte Can_backup_file() {

    if (oa.wk1_blocks > 0)
        return _TRUE;
    return _FALSE;

} /* can_backup_file */



/*
    BACKUP_FILE                            NEED TO READ PRIOR BLOCK FROM WK1
                                        IF ANY. RETURN TRUE IF BACKUP DONE.

*/
byte Backup_file() {
    wbyte i;
    pointer saver;
    word saver2;

    saver = oa.toff[ed_tagw];
    saver2 = oa.tblock[ed_tagw];

    if (oa.wk1_blocks == 0)
        return _FALSE;/* CANNOT BACKUP */
    cursor = oa.low_s;                        /* MAKE ROOM FOR NEW RECORD */
    Re_window();
    Check_window(oa.block_size);            /* MAY NEED MORE ROOM */
    if (Read_previous_block(oa.wk1_conn, oa.low_e, _FALSE))
        oa.low_e = oa.low_e + oa.block_size;
    cursor = oa.low_e;
    for (i = 1; i <= num_tag; i++) {                    /* ADJUST TAGS FOR REMOVED BLOCK */
        if (oa.tblock[i] >= oa.wk1_blocks && oa.tblock[i] != 0xffff)
            oa.tblock[i] = oa.tblock[i] - 1;
        else if (oa.tblock[i] == oa.wk1_blocks - 1)
            oa.toff[i] = ((word)oa.toff[i]) + oa.low_s;
    }
    oa.wk1_blocks = oa.wk1_blocks - 1;            /* ADJUST WK1 BLOCK COUNT */

    if (in_other != w_in_other) {
        oa.toff[ed_tagw] = saver;
        oa.tblock[ed_tagw] = saver2;
    }

    if (w_in_other == in_other)
        w_dirty = _TRUE;

    return _TRUE;

} /* backup_file */




/*
    CAN_FORWARD_FILE                    TRUE IF THERE IS MORE INPUT ON DISK
*/
byte Can_forward_file() {

    if (oa.wk2_blocks > 0 || oa.more_input)
        return _TRUE;
    return _FALSE;

} /* can_forward_file */



/*
    FORWARD_FILE                        NEED TO READ NEXT BLOCK FROM WK2 OR
                                        INPUT FILE. RETURN TRUE IF FORWARD DONE.
*/
byte Forward_file() {
    wbyte i;
    word numin;
    pointer saver;
    word saver2;

    saver = oa.toff[ed_tagw];
    saver2 = oa.tblock[ed_tagw];

    if (Can_forward_file()) {            /* WE HAVE HAVE MORE TEXT */

        cursor = oa.high_e;                    /* MAKE ROOM FOR NEW RECORD */
        Re_window();
        Check_window(oa.block_size);            /* MAY NEED MORE ROOM */

    /*    MOVE TAGS DOWN SO READ WILL PUT DATA ABOVE TAGS    */
        for (i = 1; i <= num_tag; i++) {
            if (oa.tblock[i] == oa.wk1_blocks && oa.toff[i] == oa.high_s)
                oa.toff[i] = oa.low_e;
        }
        for (i = first_text_line; i <= message_line; i++) {
            if (have[i] == oa.high_s)
                have[i] = oa.low_e;
        }

        cursor = oa.low_e;
        if (oa.wk2_blocks > 0) {            /* GET SPILL FIRST */
            if (Read_previous_block(oa.wk2_conn, oa.low_e, _FALSE))
                oa.low_e = oa.low_e + oa.block_size;
            for (i = 1; i <= num_tag; i++) {                /* ADJUST TAGS FOR REMOVED BLOCK */
                if (oa.tblock[i] > oa.wk1_blocks && oa.tblock[i] != 0xffff) {
                    oa.tblock[i] = oa.tblock[i] - 1;
                    if (oa.tblock[i] == oa.wk1_blocks)
                        oa.toff[i] = ((word)oa.toff[i]) + cursor;
                }
            }
            oa.wk2_blocks = oa.wk2_blocks - 1;        /* ADJUST WK1 BLOCK COUNT */
            if (oa.wk2_blocks == 0 && oa.more_input == _FALSE) {
                Add_eof();                /* NEED | ADDED AT END OF FILE */
                Clean_tags();            /* TAG AT EOF MUST MOVE FROM
                                                LOW_E TO HIGH_S */
            }
        }
        else {                            /* GRAB MORE INPUT */
            dword dtemp;

            Working();

            if (oa.file_disposition == view_only) {
                dtemp = oa.block_size * oa.wk1_blocks + Size_of_text_in_memory();
                dq_seek(files[in_file].conn, abs_mode, dtemp, &excep);
                Echeck();
            }
            if ((numin = Read(in_file)) < oa.block_size) {
                Detach_input();
                Add_eof();
            }
        }
        Re_window();                        /* LEAVE WITH NEW BLOCK AFTER
                                            AFTER WINDOW */
        if (in_other != w_in_other) {
            oa.toff[ed_tagw] = saver;
            oa.tblock[ed_tagw] = saver2;
        }
        else {
            w_dirty = _TRUE;
        }

        return _TRUE;
    }

    return _FALSE;
} /* forward_file */

// #eject
/*        THE FOLLOWING DOES THE WRITE TO SIDE FILES    */




/*
    WRITE                                WRITE. TO FILE, FROM, LENGTH.
*/
void Write(byte nfile, pointer buf, word len) {

    if (len > 0) {

        Working();

        if (config != SIIIE)
            Check_for_keys();
        excep = fwrite(buf, 1, len, files[nfile].conn) != len ? errno : 0;
        Echeck();
        if (config != SIIIE)
            Check_for_keys();
    }
} /* write */




/*
    WRITE_TO_TAGI                        WRITE TEXT BETWEEN CURSOR AND TAGI
                                        TO DESIGNATED FILE. IF FILE IS BLOCK
                                        FILE, MUST COUNT BYTES WRITTEN.
*/

static void Write_to_tagi(byte nfile, byte do_delete) {
    byte i, target_tag;
    word size_out;

    if (nfile == block_file) {
        if (files[block_file].conn == NULL)   /* FILE NOT OPEN */
            Openo(block_file, update_mode, 2);
        else
            Open_block(output_mode);
    }
    else {
        Openo(util_file, output_mode, 2);
    }
    if (excep != 0)
        return;
    block_put_file_attached = _TRUE;


    Set_tag(ed_taga, oa.high_s);    /* JUMP TO START OF AREA */
    target_tag = ed_tagi;                    /* AND SET TARGET_TAG TO HIGH TAG */
    if (oa.tblock[ed_tagi] < oa.wk1_blocks ||
        (oa.tblock[ed_tagi] == oa.wk1_blocks && oa.toff[ed_tagi] < oa.low_e)) {
        Jump_tagi();
        target_tag = ed_taga;
    }

    /*    KEEP WRITING UNTIL THE LAST BYTE IS IN MEMORY */

    if (nfile == block_file)
        block_low = block_high = 0;

    while (oa.tblock[target_tag] > oa.wk1_blocks) {
        size_out = (word)(oa.high_e - oa.high_s);
        Write(nfile, oa.high_s, size_out);
        if (nfile == block_file) {
            if (block_low + size_out > 0xffff)
                block_high++;
            block_low += size_out;
        }
        /*    IF DELETE FLAG IS ON THENDO THE DELETE    */
        if (do_delete) {
            oa.high_s = oa.high_e;
            Clean_tags();
        }
        i = Forward_file();
    }

    /*    WRITE REST OF TEXT  BUT NOT THE EOF MARKER */

    size_out = (word)(oa.toff[target_tag] - oa.high_s);
    Write(nfile, oa.high_s, size_out);
    if (nfile == block_file) {
        if (block_low + size_out > 0xffff)
            block_high++;
        block_low = block_low + size_out;
        Close_block();
    }

    /*    DO NOT DELETE IN MEMORY BLOCK AS THIS WILL BE DONE BY A SUBSEQUENT CALL
        TO DELETE_TO_TAGI    */

        /*    NOW GO BACK TO START AND DELETE TAG A */

    Jump_tag(ed_taga);
} /* write_to_tagi */




/*
    WRITE_BLOCK_FILE                    WRITE OUT TO THE BLOCK FILE
*/
void Write_block_file(byte do_delete) {

    Write_to_tagi(block_file, do_delete);
} /* write_block_file */




/*
    WRITE_UTIL_FILE                    WRITE OUT TO THE UTIL FILE
*/
void Write_util_file() {
    block_put_file_attached = _FALSE;
    Write_to_tagi(util_file, _FALSE);
    if (block_put_file_attached)
        Detach(util_file);
} /* write_util_file */




/*
    READ_UTIL_FILE                    READ  A NAMED FILE INTO THE TEXT. THE
                                    NAME IS STILL IN THE INPUT_BUFFER.
*/

void Read_util_file() {
    word numin;

    Openi(util_file, 2);
    Echeck();
    if (excep != 0) return;

    /*    NOW HAVE THE FILE READY TO READ. READ THE FILE COUNT TIMES. */
    /*    IT WOULD BE BETTER TO SPECIAL CASE THE SITUATION IN WHICH THE */
    /*    FILE IS 'SMALL' AND COUNT IS OVER ONE BUT THAT SITUATION IS */
    /*    PROBABLY NOT COMMON */

    Set_tagi_at_lowe();
    while (count > 0 && cc_flag == _FALSE) {
        Rewind(util_file);
        if (excep != 0) return;
        numin = oa.block_size;
        while (numin == oa.block_size) {
            Check_window(oa.block_size);
            numin = Read(util_file);
            if (excep != 0) return;
        }
        count--;
    }
    Jump_tagi();
    Detach(util_file);
} /* read_util_file */




/*
    READ_BLOCK_FILE                    READ  THE BLOCK FILE INTO THE TEXT.
*/

void Read_block_file() {
    word read_low, read_high, numin;


    /*    NOW HAVE THE FILE READY TO READ. READ THE FILE COUNT TIMES. */
    /*    IT WOULD BE BETTER TO SPECIAL CASE THE SITUATION IN WHICH THE */
    /*    FILE IS 'SMALL' AND COUNT IS OVER ONE BUT THAT SITUATION IS */
    /*    PROBABLY NOT COMMON */
    /*    FOR THE BLOCK FILE, READING PAST EOF IS POSSIBLE SINCE THE FILE MAY
        HAVE LEFT OVER JUNK IN IT    */

    Set_tagi_at_lowe();
    while (count > 0 && cc_flag == _FALSE) {
        Open_block(input_mode);
        if (excep != 0)
            return;
        numin = oa.block_size;
        /*    READ_HIGH AND READ_LOW CONTAIN 4 BYTE NUMBER OF BYTES LEFT TO READ */

        read_high = block_high;
        read_low = block_low;
        while (numin == oa.block_size) {
            if (numin > read_low && read_high == 0)
                numin = read_low;
            Check_window(numin);
            if (numin > 0) {
                if (config != SIIIE)
                    Check_for_keys();
                numin = (byte)fread(oa.low_e, 1, numin, files[block_file].conn);
                excep = ferror(files[block_file].conn) ? errno : 0;
                if (excep == 0)
                    oa.low_e = oa.low_e + numin;
                else {
                    Echeck();
                    return;
                }
                if (config != SIIIE)
                    Check_for_keys();
            }
            /*    DECREMENT 4 BYTE INTEGER IS READ_LOW AND READ_HIGH    */
            if (read_low < numin)
                read_high--;
            read_low -= numin;
        }
        count--;
        Close_block();
    }
    Jump_tagi();
} /* read_block_file */




    /* TEST CONTROLC FLAG AND CLEAN UP MACROS AND FLAG IF IT IS ON. */

byte Have_controlc() {

    if (!cc_flag)
        return _FALSE;
    if (in_macro_def) {
        in_macro_def = _FALSE;
        Kill_message();        /* REMOVE 'Macro' FROM MESSAGE */
    }
    if (macro_exec_level > 0) {
        macro_exec_level = 0;
        Rebuild_screen();
    }
    return _TRUE;

} /* have_controlc */

