// #SMALL
// #title('COMAND                         INPUT A COMMAND')
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

static boolean At_crlf(pointer str_p, byte i, boolean left);

boolean pause_before_continue = _TRUE;
/* don't wait to a space in hit space */

byte hit_space_text[] = { "\x17" RVID "hit space to continue " };
byte ed_mess[] = { "\x11" "<HOME> to re-edit" };

#define single_char	0       /* single char input. */
#define from_input_line	1   /* line editing. */
#define from_find	2       /* input target string. */
#define from_replace	3   /* input replacement string. */

boolean in_input_line = _FALSE;
struct {
    boolean reediting;  /* reediting old string. */
    byte mode;         /* single_char, from_input_line, from_find,
                              from_replace */
    byte last_cursor_key;  /* tells where to go when <HOME> is typed. */
} edit_stat = { _FALSE, single_char, left_code };

byte cur_pos; /* CURRENT POSITION OF CURSOR WITHIN INPUT BUFFER */

byte prompt_len;


boolean reediting_replacement, reediting_target;
/* used to determine if reediting when switching
   between the target string and the replacement string. */


boolean strip_parity = _TRUE;




byte Upper(byte ch) {

    if (ch >= 'a' && ch <= 'z')
        ch = ch - ('a' - 'A');
    return ch;
} /* upper */



/* RETURN TRUE IF CHARACTER IS ILLEGAL, OTHERWISE RETURN FLASE. */
byte Is_illegal(byte ch) {

    if ((ch >= 0x7f) || (ch < ' ' && (ch != TAB && ch != LF && ch != CR)))
        return _TRUE;
    return _FALSE;
} /* is_illegal */



/* INPUT COMMAND CHARACTER. PROGRAMMERS WITH
   DELICATE SENSIBILITIES SHOULD SKIP THIS ROUTINE. */

byte Cmd_ci() {

    byte ch;
    wbyte i;
    match_t matched;
    static dword mask[5] = { 0, 0xff, 0xffff, 0xffffff, 0xffffffff };
    match_t *ibase;
    byte last_coded;

    if (Have_controlc())
        return CONTROLC;

    /* SEE IF EXECUTING A MACRO */
    ch = Supply_macro_char();
    if (macro_exec_level > 0) {
#ifdef UNIX
        if (ch == LF)
            ch = CR;
#endif
        return ch;    /* YES, GOT CHAR FROM MACRO */
    }

    last_coded = last_code - first_code;

start_over:

    ch = Ci();

    if (strip_parity)
        ch &= 0x7f;     /* STRIP PARITY */

    if (Have_controlc())
        return CONTROLC;
    matched.length = 0;
    ibase = (match_t *)input_codes;
    while (true) {
        /* assign the character read in to the matching string */
        matched.code[++matched.length] = ch;

        bool havePrefix = false;
        for (i = 0; i <= last_coded; i++) {
            // check for length & prefix match
            if (matched.length <= ibase[i].length && ((matched.dw ^ ibase[i].dw) & mask[matched.length]) == 0) {
                if (matched.length == ibase[i].length) {
                    if (i == ignore_code - first_code)
                        goto start_over;
                    else {
                        ch = i + first_code;
                        goto got_a_char;
                    }
                } else
                    havePrefix = true;
            }
        }
        if (havePrefix && matched.length != 4) {
            ch = Ci();                             // try rest of prefix
            if (strip_parity)
                ch &= 0x7f;
        } else if (matched.length != 1) {         // at least one char was previously matched so error
            AeditBeep();
            goto start_over;
        } else
            break;                              // first char doesn't match so not coded
    }


got_a_char:
#ifdef UNIX
    if (ch == LF)
        ch = CR;
#endif

    /* FOUND AN INPUT CHARACTER. SEE IF IT MUST BE SAVED AS PART OF A MACRO */
    if (in_macro_def) {
        if (!Add_macro_char(ch)) {
            Handle_controlc();
            return CONTROLC;
        }
    }
    return ch;

} /* cmd_ci */






/**************************************************************************/
/*                          Not_Cursor_Movement                           */
/* TRUE IF LAST_CMD IS NOT UP OR DOWN OR CR.                              */
/*                                                                        */
/**************************************************************************/
byte Not_cursor_movement(byte cmd) {

    /*BEGIN*/
    return (cmd < up_code || cmd > home_code) && cmd != CR;
} /* not_cursor_movement */




/**************************************************************************/
/*                             get_hexin                                  */
/*                                                                        */
/**************************************************************************/
byte Get_hexin(byte *err_p) {

    byte ch, i, num;
    ;
    /*BEGIN*/
    num = 0;
    Print_message("\x5" "<HEX>");
    for (i = 1; i <= 2; i++) {
        ch = Cmd_ci();
        if (ch == CONTROLC) {
            *err_p = CONTROLC;
            Clear_message();
            return 0;
        }
        ch = Hex_value(ch);
        if (ch != not_hex) {
            num = num * 16 + ch;
        }
        else {
            Error(invalid_hex);
            *err_p = 1; /* error */
            return 0;
        }
    }
    *err_p = 0; /* no error */
    Clear_message();
    return num;
} /* get_hexin */



/**************************************************************************/
/*                            Add_Three                                   */
/*                                                                        */
/**************************************************************************/
nestedProc void Add_three(byte ch) {
    Add_str_char('\\');
    Add_str_char(hex_digits[ch >> 4]);
    Add_str_char(hex_digits[ch & 0xf]);
} /* add_three */



/**************************************************************************/
/*                            add_str_special                             */
/* ADD STRING TO STRING.                                                  */
/*    IF CRLF THEN ADD <cr>.                                              */
/*    IF TAB THEN ADD <TAB>.                                              */
/*    IF UNPRINTABLE THEN \XX.                                            */
/*                                                                        */
/**************************************************************************/
void Add_str_special(pointer str_p) {
    byte ch;
    wbyte i;

    /*********************** main unit of Add_Str_Special *********************/
    /*BEGIN*/
    for (i = 1; i <= str_p[0]; i++) {
        ch = str_p[i];
        if (ch == CR) {
            if (At_crlf(str_p, (byte)i, _FALSE)) {
                Add_str_str("\x4" "<nl>");
                i++;
            }
            else {
                Add_three(ch);
            }

        }
#ifdef UNIX
        else if (ch == LF) {
            Add_str_str("\x4" "<nl>");

        }
#endif
        else if (ch == TAB) {
            Add_str_str("\x5" "<TAB>");
        }
        else if (Is_illegal(ch) || ch == LF) {
            Add_three(ch);
        }
        else {
            Add_str_char(ch);
        }
    }
} /* add_str_special */






boolean wait_in_text = _FALSE;
/* indicates that the cursor is positioned in the text, and not
   in the prompt_line when waiting to a response. */


   /**************************************************************************/
   /*                         INPUT_YES_NO                                   */
   /* ISSUE A PROMPT  AND WAIT FOR A  YES OR NO ANSWER.   THE Y_TYPE BOOLIAN */
   /* SAYS WHETHER OR NOT Y IS DEFAULT.  RETURN TRUE (IF Y), FALSE (IF N) OR */
   /* CONTROLC.                                                              */
   /*                                                                        */
   /**************************************************************************/
byte Input_yes_no(pointer prompt, byte y_type) {
    byte ch, i_row, i_col;

    /*BEGIN*/
    i_row = row;
    i_col = col;

    /*    ADD THE REVERSE VIDEO BYTE    */
    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_char(rvid);     /* NEED REVERSE VIDEO    */

    if (macro_exec_level == 0) {
        Add_str_str(prompt);
        if (y_type)
            Add_str_str("\xc" " ([y] or n) ");
        else
            Add_str_str("\xc" " (y or [n]) ");
        Print_prompt(tmp_str);
        if (wait_in_text) Put_goto(i_col, i_row);        /* PUT CURSOR BACK */
        else Put_re_col(tmp_str[0]);/* MOVE CURSOR TO END OF LINE*/
    }

    ch = Cmd_ci();                        /* GET USER RESPONSE */
    Print_prompt(&null_str);     /* delete the prompt to avoid confusion */
    Put_goto(i_col, i_row);        /* PUT CURSOR BACK */
    ch = Upper(ch);
    if (ch != CONTROLC)             // CONTROLC will be treated as TRUE
        if (y_type)
            ch = (ch != 'N') ? _TRUE : _FALSE;
        else
            ch = (ch == 'Y') ? _TRUE : _FALSE;
    return ch;

} /* input_yes_no */



/**************************************************************************/
/* Same as input_yes_no, but the answer is not taken from macro, and is   */
/* not buffered.                                                          */
/**************************************************************************/
byte Input_yes_no_from_console(pointer prompt, byte y_type, boolean in_replace) {
    byte save_macro_level;
    boolean save_macro_def;
    boolean save_force_writing;
    byte ch;


    /* In batch mode the answer to these questions is positive.
       The user can't answer because he doesn't have a prompt line. */
    if (batch_mode)
        return _TRUE;

    save_macro_level = macro_exec_level;
    macro_exec_level = 0;    /* GET ANSWER FROM ACTUAL CONSOLE */
    save_macro_def = in_macro_def;
    in_macro_def = _FALSE;           /* don't buffer the answer if in_macro_def */
    save_force_writing = force_writing;
    force_writing = _TRUE;

    if (in_replace)
        wait_in_text = _TRUE;
    else  /* dont make the user crazy in ?replace */
        AeditBeep();
    ch = Input_yes_no(prompt, y_type);
    wait_in_text = _FALSE;
    Working();

    force_writing = save_force_writing;
    in_macro_def = save_macro_def;
    macro_exec_level = save_macro_level;
    return ch;

} /* input_yes_no_from_console */





/**************************************************************************/
/*                                HIT_SPACE                               */
/* ASK USER TO HIT SPACE TO CONTINUE                                      */
/*                                                                        */
/**************************************************************************/
byte Hit_space() {

    byte i, save_macro_level;
    boolean save_force_writing;

    /*BEGIN*/
    if (!pause_before_continue)
        return ' ';
    save_macro_level = macro_exec_level;
    macro_exec_level = 0;    /* GET ANSWER FROM ACTUAL CONSOLE and force prompt*/
    save_force_writing = force_writing;
    force_writing = _TRUE;
    Print_prompt(hit_space_text);
    Put_re_col(hit_space_text[0]); /* put cursor at right position. */
    i = Ci() & 0x7f;                    /* LOOK FOR SPACE FOR PERMISSION */
    Working();
    Print_prompt(&null_str);
    force_writing = save_force_writing;
    macro_exec_level = save_macro_level;
    Co_flush();
    if (Have_controlc())
        i = CONTROLC;
    return i;
} /* hit_space */





/**************************************************************************/
/*                            add_ch                                      */
/*                                                                        */
/**************************************************************************/
nestedProc void Add_ch(byte ch) {


    /*BEGIN*/
    if (cur_pos < last(input_buffer) && input_buffer[0] < last(input_buffer)) {
        cur_pos++;
        input_buffer[0] = input_buffer[0] + 1;
        if (cur_pos != last(input_buffer)) {            // PMO replace with memmove?
            movrb(&input_buffer[cur_pos], &input_buffer[cur_pos + 1],
                last(input_buffer) - cur_pos);
        }
        input_buffer[cur_pos] = ch;
    }
    else {
        AeditBeep();
    }
} /* add_ch */



/**************************************************************************/
/*                            add_input_buffer                            */
/* ADD A CHARACTER TO INPUT_BUFFER. RETURN CHARACTER SO USER CAN HANDLE   */
/* CONTROL C.                                                             */
/*                                                                        */
/**************************************************************************/
static byte Add_input_buffer() {

    byte ch;
    byte codes1[] = { 8,
                 rubout_code,
                 delete_left_code,
                 delete_right_code,
                 delete_line_code,
                 delete_code,
                 left_code,
                 right_code,
                 undo_code };

    

    /********************* main unit of Add_Input_Buffer **********************/
    /*BEGIN*/
    ch = Cmd_ci();

    if (!edit_stat.reediting) {
        if (Find_index(ch, codes1) < 8) {
            if (ch == rubout_code && edit_stat.mode == from_replace) {
                /* return rubout only in first position of replacement string,
                   even if not reediting. */
                return rubout_code;
            }
            else {
                Clear_message();
                edit_stat.reediting = _TRUE;
            }
        }
        else if (ch == home_code) {
            Clear_message();
            edit_stat.reediting = _TRUE;
            return 0; /* any non special char */
        }
        else {
            Clear_message();
            if (ch == esc_code) return esc_code;
            edit_stat.reediting = _TRUE;
            input_buffer[0] = 0;
        }
    }
    else {
        Clear_message();
    }


    if (ch != left_code && ch != right_code && ch != home_code)
        edit_stat.last_cursor_key = 0;  /* illegal value */


    switch (ch) {  /* illegal value */


    case CONTROLC:      // CONTROL C or Escape
    case esc_code:
        return ch; 
    case rubout_code: {            /* rubout_code */
        byte x;
        if (cur_pos > 0) {
            if (At_crlf(input_buffer, cur_pos, _TRUE))
                x = 1;
            else
                x = 0;
            memcpy(&input_buffer[cur_pos - x], &input_buffer[cur_pos + 1], input_buffer[0] - cur_pos);
            input_buffer[0] = input_buffer[0] - 1 - x;
            cur_pos = cur_pos - 1 - x;
            return 0;
        }
        if (edit_stat.mode == from_replace || edit_stat.mode == single_char) /* return rubout only in first position in replace. */
            return ch;
        else
            return 0;
    }

    case n_var_code: {    /* n_var_code */
        pointer str_p;
        byte i;

        if (edit_stat.mode == single_char)
            str_p = Get_n_var(10, _FALSE); /* called from input_command. */
        else
            str_p = Get_n_var(10, _TRUE);  /* otherwise */
        if (str_p[0] == 0xff)
            return CONTROLC;
        for (i = 1; i <= str_p[0]; i++) {
            Add_ch(str_p[i]);
        }
        return ch;
    }
    // default falls through
    }




    if (edit_stat.mode != single_char) {

        switch (ch) {

        case delete_left_code: { /* delete_left_code */
            byte new_len;
            if (cur_pos > 0) {
                new_len = input_buffer[0] - cur_pos;
                ubuf_left = ubuf_used = cur_pos;
                memcpy(&ubuf[0], &input_buffer[1], ubuf_used);
                memcpy(&input_buffer[1], &input_buffer[cur_pos + 1], input_buffer[0] - cur_pos);
                input_buffer[0] = new_len;
                cur_pos = 0;
                u_is_line = _FALSE;
            }
            return 0;
        }
        case CR:	    /* CR */
            if (edit_stat.mode != from_input_line) {
#ifdef MSDOS
                Add_ch(CR);
#endif
                Add_ch(LF);
                return 0; /* a returned CR will cause termination */
            }
            /* fall through */

        case delete_right_code:	/* delete_right_code */
            if (ch == delete_right_code) {
                ubuf_left = 0;
                ubuf_used = input_buffer[0] - cur_pos;
                memcpy(&ubuf[0], &input_buffer[cur_pos + 1], ubuf_used);
                u_is_line = _FALSE;
            }
            input_buffer[0] = cur_pos;
            return ch;

        case delete_line_code:	/* delete_line_code */
            ubuf_used = input_buffer[0];
            ubuf_left = cur_pos;
            memcpy(&ubuf[0], &input_buffer[1], ubuf_used);
            input_buffer[0] = cur_pos = 0;
#ifdef MSDOS
            ubuf[ubuf_used++] = CR;
#endif
            ubuf[ubuf_used++] = LF;
            u_is_line = _TRUE;
            return 0;

        case delete_code: { /* delete_code */
            byte x;
            if (input_buffer[0] > cur_pos) {
                x = At_crlf(input_buffer, cur_pos + 1, _FALSE) ? 1 : 0;
                input_buffer[0] = input_buffer[0] - 1 - x;
                memcpy(&input_buffer[cur_pos + 1], &input_buffer[cur_pos + 2 + x], input_buffer[0] - cur_pos);
            }
            return 0;
        }

        case left_code:	/* left_code */
            if (cur_pos > 0) {
                if (At_crlf(input_buffer, cur_pos, _TRUE))
                    cur_pos = cur_pos - 2;
                else
                    cur_pos--;
            }
            else
                AeditBeep();
            edit_stat.last_cursor_key = left_code;
            return 0;

        case right_code:	 /* right_code */
            if (cur_pos < input_buffer[0]) {
                if (At_crlf(input_buffer, cur_pos + 1, _FALSE))
                    cur_pos = cur_pos + 2;
                else
                    cur_pos++;
            }
            else {
                AeditBeep();
            }
            edit_stat.last_cursor_key = right_code;
            return 0;

        case home_code: /* home_code */
            if (edit_stat.last_cursor_key == right_code) {
                cur_pos = input_buffer[0];
            }
            else if (edit_stat.last_cursor_key == left_code) {
                cur_pos = 0;
            }
            else {
                AeditBeep();
            }
            return 0;

        case undo_code:	     /* undo_code */
        case s_var_code: {    /* s_var_code */

            word i;
            pointer char_base;
            word limit;
            byte save_ch, save_cur_pos;

            save_ch = ch;
            /* set up parameters for loop */
            if (ch == s_var_code) {
                char_base = Get_s_var();
                if (char_base[0] == 0xff) /* CONTROLC was typed instead of second char*/
                    return CONTROLC;
                limit = char_base[0];
                char_base = &char_base[1];
            }
            else {
                char_base = ubuf;
                limit = ubuf_used;
            }
            save_cur_pos = cur_pos;
            i = 0;
            while (i < limit) {
                ch = char_base[i];
#ifdef MSDOS
                if (save_ch == undo_code && ch == CR)
#else
                if (save_ch == undo_code && ch == LF)

#endif
                    break;
                Add_ch(ch);
                i++;
            }
            if (save_ch == undo_code)
                /* cursor position in undo depends on how many characters
                   were to the left of the cursor when we did the delete */
                cur_pos = save_cur_pos + ubuf_left;
            if (cur_pos > input_buffer[0])
                cur_pos = input_buffer[0];
            return 0;
        }
        case hexin_code: {    /* hexin_code */

            byte err;
            ch = Get_hexin(&err);
            if (err == CONTROLC) /* CONTROLC was typed instead of second or third char*/
                return CONTROLC;
            if (err != 0) /* error */
                return 0;
            Add_ch(ch);
            return 0;
        }

        case up_code:  /* up_code */
            AeditBeep();
            return 0;
        case down_code:   /* down_code */
            AeditBeep();
            return 0;
        case macro_exec_code: /* macro_exec_code */
            AeditBeep();
            return 0;
        // default falls through
        }
    }

    Add_ch(ch);
    return ch;

} /* add_input_buffer */







/**************************************************************************/
/*                            collect_count                               */
/*                                                                        */
/**************************************************************************/
static void Collect_count() {

    byte old_row, old_col;
    byte ch;
    byte i;
    /*BEGIN*/
    count = 1;     /* Default */
    infinite = _FALSE;

    if ((command == '/') || (command == n_var_code) ||
        (command >= '0' && command <= '9')) {
        /* REMEMBER CURRENT LOCATION    */
        old_row = row;
        old_col = col;

        while (1) {
            if (command == CONTROLC || command == esc_code)
                break;
            if ((input_buffer[1] == '/') && (input_buffer[0] != 0)) {
                if (input_buffer[0] >= 2) {
                    count = 0;
                    infinite = _TRUE;
                    break;
                }
            }
            else if ((input_buffer[1] >= '0') && (input_buffer[1] <= '9')
                && (input_buffer[0] != 0)) {
                count = 0;
                for (i = 1; i <= input_buffer[0]; i++) {
                    ch = input_buffer[i];
                    if (ch >= '0' && ch <= '9') {
                        if (i > 5) {
                            AeditBeep();
                            input_buffer[0] = 5;
                            cur_pos = 5;
                        }
                        else {
                            dword dtemp;
                            dtemp = count * 10 + ch - '0';
                            if (dtemp > 0xffff) {
                                AeditBeep();
                                input_buffer[0] = 4;
                                cur_pos = 4;
                            }
                            else {
                                count = dtemp;
                            }
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            else if (input_buffer[0] != 0) {
                count = 1;
                break;
            }
            Print_count_message();
            Put_re_col(line_size[message_line]);
            /* NEEDED BY SERIES II AFTER RUBOUT */
            command = Add_input_buffer();
        }

        /* PUT CURSOR BACK INTO THE TEXT */
        Put_goto(old_col, old_row);
    }

} /* collect_count */





/**************************************************************************/
/*                            input_command                               */
/* INPUT A  COMMAND.  A DECIMAL REPETITION  COUNT (IF ANY) IS ACCUMULATED */
/* ON  THE MESSAGE LINE.   THE COMMAND LETTER IS  RETURNED AFTER COUNT IS */
/* CALCULATED.   THE PARAMETER IS THE ADDRESS OF THE PROMPT STRING.  SOFT */
/* KEYS ARE TRANSLATED BY SCANNING THE PROMPT.                            */
/*                                                                        */
/**************************************************************************/
byte Input_command(pointer prompt) {

    /*BEGIN*/
    Init_str(tmp_str, sizeof(tmp_str));
    input_buffer[0] = cur_pos = 0;

    last_cmd = command;

    if (Not_cursor_movement(last_cmd)) Print_prompt_and_repos(prompt);
    Clear_count_message();

    edit_stat.mode = single_char;
    edit_stat.reediting = _TRUE;

    command = Add_input_buffer();            /* GET FIRST CHARACTER OF INPUT */

    /* CLEAR MESSAGE UNLESS IN QUIT COMMAND OR IN THE PROCESS OF SCROLLING */
    if (/* (not_cursor_movement(command)) AND   IB !!! */  (last_cmd != 'Q')) Clear_message();

    Collect_count();  /* Accumulate count (if any) */

    command = Upper(command);

    if (command != home_code && !(last_main_cmd == home_code && command == 'A'))
        last_move_command = 0; /* undefined */

    return command;

} /* input_command */







/**************************************************************************/
/*                        at_CRLF                                         */
/* Params: a pointer to string, index of current byte, and a flag.        */
/*  If the flag indicates "left" then check if current byte is a LF       */
/*  and prev byte is CR; else check if current byte is CR and the next    */
/*  is LF. Helps to handle the special "char" CRLF.                       */
/**************************************************************************/
static boolean At_crlf(pointer str_p, byte i, boolean left) {
#ifdef MSDOS
    if (left) {
        if (str_p[i] == LF && str_p[i - 1] == CR && i > 0) return _TRUE;
    }
    else {
        if (str_p[i] == CR && str_p[i + 1] == LF && i < str_p[0]) return _TRUE;
    }
#endif
    return _FALSE;

} /* at_crlf */






/**************************************************************************/
/**************************************************************************/
void Print_input_line() {

    word pos;    /* word - because the length of 60 TABs is 300 (>256) */
    byte i, ch, num;
    /*BEGIN*/


    if (tmp_str[0] > 79) {
        tmp_str[0] = 79;
        tmp_str[79] = '!';
    }
    Print_prompt(tmp_str);

    if (macro_exec_level == 0) {

        /* MOVE CURSOR TO CURRENT POSITION */
        i = 1; pos = 0;
        while (i <= cur_pos) {
            ch = input_buffer[i];
            if (ch == CR) {
                if (At_crlf(input_buffer, i, _FALSE)) {
                    num = 4;
                    i++;
                }
                else {
                    num = 3;
                }

            }
#ifdef UNIX
            else if (ch == LF) {
                num = 4;
            }
#endif
            else if (ch == TAB) {
                num = 5;
            }
            else if (Is_illegal(ch) || ch == LF) {
                num = 3;
            }
            else {
                num = 1;
            }
            pos = pos + num;
            i++;
        }
        pos = pos + prompt_len;
        if (pos > 78) pos = 78;
        Put_re_col((byte)pos);
    }

} /* print_input_line */




/**************************************************************************/
/*                            input_l                                     */
/*                                                                        */
/**************************************************************************/
static byte Input_l(pointer prompt_p, pointer prev_string_p) {

    /* the reentrancy is because of the following array (saves space).*/
    byte tmp_prompt[81];
    byte ch;
    ;
    /*BEGIN*/
    cur_pos = 0;

    /* put old string to be reedited in input_buffer. */
    if (prev_string_p[0] > string_len)
        prev_string_p[0] = string_len;
    Move_name(prev_string_p, input_buffer);

    /* There is a danger that prompt_p is in tmp_str, so it must be saved
       in tmp_prompt or the next init_str will destroy it. */
    Move_name(prompt_p, tmp_prompt);

    Init_str(tmp_str, 81);
    Add_str_char(rvid);   /* reverse video */
    Add_str_special(tmp_prompt);
    /* the constant part of the edit prompt_p. */
    prompt_len = tmp_str[0];
    /* mark the portion of prompt_p to be updated while editing. */
    Add_str_special(input_buffer);
    if (edit_stat.mode == from_find || edit_stat.mode == from_replace)
        Add_str_char('"');

    if (edit_stat.mode == from_find && edit_stat.reediting)
        /* The above condition indicates that we
           returned from replacement to target string,
           and the cursor must be at end of string. */
        cur_pos = input_buffer[0];
    else
        cur_pos = 0;
    if (input_buffer[0] == 0)
        edit_stat.reediting = _TRUE;
    if (!edit_stat.reediting)
        Print_message(ed_mess);
    else Print_message(&null_str);

    edit_stat.last_cursor_key = 0; /* 0 = undefined */
    in_input_line = _TRUE;

    while (1) {
        Print_input_line();
        ch = Add_input_buffer();
        if (ch == CONTROLC) {
            in_input_line = _FALSE;
            return CONTROLC;
        }
        /* update prompt_p */
        tmp_str[0] = prompt_len;
        Add_str_special(input_buffer);
        if (edit_stat.mode == from_find || edit_stat.mode == from_replace)
            Add_str_char('"');
        if ((ch == CR) || (ch == esc_code) || (ch == rubout_code)) {
            Print_input_line();
            Co_flush();
            if (edit_stat.reediting) {
                if (edit_stat.mode == from_replace)
                    reediting_replacement = _TRUE;
                else
                    reediting_target = _TRUE;
            }
            in_input_line = _FALSE;
            return ch;
        }
    }

} /* input_l */






/**************************************************************************/
/*                            input_line                                  */
/*                                                                        */
/**************************************************************************/
byte Input_line(pointer prompt, pointer prev_string_p) {

    /*BEGIN*/
    edit_stat.reediting = _FALSE;
    edit_stat.mode = from_input_line;
    return Input_l(prompt, prev_string_p);
} /* input_line */







/**************************************************************************/
/*                            input_filename                              */
/*                                                                        */
/**************************************************************************/
byte Input_filename(pointer prompt, pointer prev_string_p) {

    byte ch;
    byte i;
    /*BEGIN*/
    ch = Input_line(prompt, prev_string_p);
    i = 1;
    /* check if a blank line */
    while ((input_buffer[i] == ' ' || input_buffer[i] == TAB)
        && i <= input_buffer[0]) {
        i++;
    }
    if ((ch != CONTROLC) && (input_buffer[0] != i - 1))
        Move_name(input_buffer, prev_string_p);
    return ch;
} /* input_filename */





/**************************************************************************/
/*                            input_fr                                    */
/*                                                                        */
/**************************************************************************/
byte Input_fr() {

    byte fr_str[25];
    byte tmp_target[string_len_plus_1];
    byte tmp_replacement[string_len_plus_1];
    /* the above two variables are used to allow references to the
       previous contents of target and replacement during replace. */
    byte a;
    /*BEGIN*/
    Init_str(fr_str, sizeof(fr_str));
    if (command == 'F') { /* 'F' or '-' */
        if (minus_type) Add_str_char('-');
        Add_str_str("\x5" "Find ");
    }
    else {
        if (command == '?')
            Add_str_char('?');
        Add_str_str("\x8" "Replace ");
    }
    Add_str_char('{');
    if (token_find)
        Add_str_str("\x3" "Tk ");
    if (!find_uplow)
        Add_str_str("\x3" "Cs ");
    if (show_find)
        Add_str_str("\x3" "Sh ");
    if (fr_str[fr_str[0]] == ' ')
        fr_str[0] = fr_str[0] - 1;
    Add_str_str("\x3" "} \"");

    Move_name(target, tmp_target);
    Move_name(replacement, tmp_replacement);
    reediting_target = _FALSE;
    reediting_replacement = _FALSE;
f: edit_stat.reediting = reediting_target;
    edit_stat.mode = from_find;
    if (Input_l(fr_str, tmp_target) == CONTROLC)
        return CONTROLC;
    Move_name(input_buffer, tmp_target);
    if (command == 'R' || command == '?') {
        Init_str(tmp_str, 81);
        Add_str_str(fr_str);
        Add_str_special(tmp_target);
        Add_str_str("\x8" "\" with \"");
        edit_stat.reediting = reediting_replacement;
        edit_stat.mode = from_replace;
        a = Input_l(tmp_str, tmp_replacement);
        if (a == CONTROLC) return CONTROLC;
        Move_name(input_buffer, tmp_replacement);
        if (a == rubout_code) { /* return to the "target" string */
            if (!edit_stat.reediting) {
                Clear_message();
                edit_stat.reediting = _TRUE;
            }
            goto f;
        }
    }
    Move_name(tmp_target, target);
    Move_name(tmp_replacement, replacement);
    return 0; /* anything except ^C */

} /* input_fr */
