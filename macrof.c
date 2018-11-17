// #SMALL
// #title ('MACROF                     process MACRO FILE')
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
#include "unix.h"
#include <string.h>

static void Macro_null();
static void Macro_save();
static void Macro_create();
static void Macro_get();
static void Macro_list();
static void Macro_insert();

/*
 * Begin iRMX II.4 Fix, Part 4 of 8, 12/28/88
 * Feature addition allowing aedit to capture CLI terminal name and then read
 * :config:termcap for initial terminal configurations based on this name.
 */
 /*
  * End iRMX II.4 Fix, Part 4 of 8
  */


byte memory[memory_leng];

logical check_for_run_keys = { _TRUE };


/* Re-Used Strings */

byte bad_command[] = { "\xb" "bad command" };
byte bad_hex_value[] = {"\xd" "bad hex value" };
byte no_such_macro[] = { "\xd" "no such macro" };

/*    LOCALS USED DURING MACROFILE PROCESSING    */

/*    THE MACROS ARRAY COMTAINS ALL CURRENT MACRO DEFINITIONS    */

/*    EACH MACRO HAS THE FOLLOWING:
        BYTE    LENGTH OF MACRO NAME.
        BYTES    MACRO NAME.
        WORD    LENGTH OF MACRO TEXT.
        BYTES    MACRO TEXT.    */

        /*    THE FOLLOWING IS A DUMMY MACRO CONTAINING ONLY THE ESCAPE CODE */
        /*    IT IS USED WHEN MACROS ARE EXECUTING AND A MOVE OFF OF THE END */
        /*    OF TEXT OR SIMILAR STOPPER IS ENCOUNTERED. ITS USE HAS THE EFFECT OF */
        /*    ENDING ANY MODE. */

struct {
    word len;
    byte contents;
} escape = { 1, esc_code };

logical next_char_is_escape = { _FALSE };

pointer macros = memory;        /* CONTAINS MACROS - memory is allocated statically */
word macro_buf_size = { 3072 };

pointer macro_at;            /* USEFUL POINTER TO MACRO LENGTHS    */
#define macro_name_length   (*macro_at)             /* USED TO GET MACRO TEXT LENGTHS    */
#define macro_text_length   (*(wpointer)macro_at)   /* USED TO GET MACRO NAME LENGTHS */
pointer macro_end = memory;       /* NEXT FREE BYTE OF MACROS */




pointer macro_next;         /*    ADDRESS OF NEXT BYTE TO PLUCK FROM MFILE */
pointer macro_last;         /*    ADDRESS 1 PAST END OF CURRENT BUFFER */
word macro_line = 0;        /*    MACRO FILE LINE NUMBER    */
boolean macro_fail = _FALSE;
byte found_em;             /* !!! CAUTION:  0FFH if found \MM, 1 if found \EM, else 0 */

macrotext_t *new_macro;             /* IF IN_MACRO THEN THIS POINTS TO THE
                             WORD THAT DEFINES THE NEW MACRO'S TEXT LENGTH    */
boolean creating = { _FALSE }; /* TRUE when creating interactively. */

/*    EIGHT LEVEL STACK ALLOWS 8 LEVELS OF MACRO EXECUTION NESTING    */

    /*    MACRO_EXEC_LEVEL CONTAINS THE NUMBER OF THE CURRENT LEVEL    */

macrotext_t *macro_exec_pointer[9];   /* POINT TO MACRO_TEXT_LENGTH BYTES FOR EACH LEVEL */
word macro_exec_length[9];    /* NUMBER OF WORDS USED UP AT EACH LEVEL */
word macro_exec_count[9];    /* NUMBER OF TIMES TO REPEAT MACRO AT A LEVEL*/
byte macro_exec_infinite[9];   /* whether or not count is infinite */
byte macro_end_state[9];     /* STATE TO RETURN TO ON END (I,X,NULL) */

dword cnt_exe[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
/* execution counter. size is 10 to save a check in macro_add_level.*/

boolean go_flag[9];

word namepair;

dispatch_t m_dispatch[8] = {
    esc_code, Macro_null,
    CONTROLC, Macro_null,
    'C', Macro_create,
    'G', Macro_get,
    'I', Macro_insert,
    'L', Macro_list,
    'S', Macro_save,
    0, 0 };

byte no_more_room[] = { "\x17" "no more room for macros" };
byte create_while_exec[] = { "\x37" "macro redefinition is forbidden while executing a macro" };


byte state = { 0 };
logical dont_double_backslash = { _FALSE };

boolean null_macro_filename = { _FALSE };

/* if there is an INIT macro, print the banner when it terminates. */
boolean do_print_banner;


/* ADDS 'error in nnn ' TO START OF ERROR MESSAGE AND THEN PRINTS IT.
   ALL ERRORS DETECTED DURING MACROFILE PROCESSING ARE PRINTED THIS WAY. */

void Macro_file_error(pointer string) {
    pointer prev_char_ptr;
    byte ch;
    byte macro_error_string[81];

    Init_str(macro_error_string, sizeof(macro_error_string));
    Add_str_str("\xe" "error in line ");
    Add_str_num(macro_line, 10);    /* Line number */
    Add_str_str("\x2" ": ");
    Add_str_str(string);

    error_status.from_macro_file = _TRUE;
    Error(macro_error_string);
    error_status.from_macro_file = _FALSE;
    if (!error_status.key_was_space) {
        macro_fail = _TRUE;
        return;
    }

    prev_char_ptr = macro_next - 1;
    if (*prev_char_ptr != ';' && *prev_char_ptr != CR) {
        while ((ch = Macro_not_blank()) != ';') {  /* scan till semicolon */
            if (macro_fail) return;
        }
    }

} /* macro_file_error */





byte bad_msg[] = { "\x0c" "bad __ value" };

static void Bad_value(pointer p) {

    bad_msg[5] = p[0];
    bad_msg[6] = p[1];
    Macro_file_error(bad_msg);
} /* bad_value */



/*
    MACRO_BYTE                        RETURN NEXT RAW BYTE FROM MACRO FILE.
                                    SET MACRO_EOF IF END OF FILE IS DETECTED.
                                    WHEN MACRO_EOF SET THEN RETURN 0FFH AS
                                    CALLERS WILL EVENTUALLY FIGURE OUT THAT
                                    SOMETHING IS NOT RIGHT. NOTICE THAT
                                    ROUTINE IS WRITTEN SO THAT CALLERS
                                    CAN BACK UP ONCE BY DECREMENTING
                                    MACRO_NEXT.

*/

byte Macro_byte() {
    byte ch;

    if (macro_fail) return 0xff;

    /*    IF CURRENT BUFFER IS EMPTY, ATTEMPT TO REFILL    */

    if (macro_next == macro_last) {
        if (!null_macro_filename) {
            macro_next = oa.low_e;
            macro_last = oa.low_e + Macro_file_read(); /*macro_file_read in io.plm*/
        }
        else {
            if (macro_next == oa.low_e) {
                macro_next = oa.high_s;        /* NEED PORTION ABOVE WINDOW */
                macro_last = oa.high_e - 1;        /* ASSUME AT EOF */
            }
        }
        if (macro_next == macro_last) {
            macro_fail = found_em = _TRUE;
            return 0xff;
        }
    }

    /*
     * Begin iRMX II.4 Fix, Part 5 of 8, 12/28/88
     * Feature addition allowing aedit to capture CLI terminal name and then read
     * :config:termcap for initial terminal configurations based on this name.
     */
     /*
      * End iRMX II.4 Fix, Part 5 of 8
      */

    ch = *macro_next;
    if (ch == LF)
        macro_line++;
    macro_next++;
    return ch;
} /* macro_byte */






/*        PROCESS$MACRO$COMMENT                loop until EOF or a *\
                                            If EOF found first, issue error
                                            message and return 0FFH
*/

static void Process_macro_comment() {
    byte ch, ch2;

    ch = 0;
    /* Scan for matching '*\' */
    while (_TRUE) {
        ch2 = Macro_byte();
        if (ch == '*' && ch2 == '\\')
            return;
        ch = ch2;
        if (macro_fail) {
            Macro_file_error("\xc" "missing '*\\'");
            return;
        }
    }

} /* process_macro_comment */




/*
    MACRO_CHAR                        THIS ROUTINE RECOGNISES THE FUNNY MACRO
                                    FILE CHARACTERS SUCH AS \0D AND \CU.
                                    IT RETURNS THE NEXT CHARACTER. CR AND
                                    LF ARE IGNORED.
*/

static byte Macro_char() {
    byte ch, ival;
    wbyte i;

next_macro_char:

    ch = Macro_byte();
    while (ch == LF || ch == CR) {
        ch = Macro_byte();
    }

    if (ch == '\\') {            /* '\' IS LEAD IN FOR STRANGE CHARS */
        ch = Macro_byte();

        if (ch == '\\') {            /* COULD BE DOUBLED '\'    */
            /* do nothing */
        }
        else if (ch == '*') {    /* Macro Comment */
            Process_macro_comment();
            if (macro_fail)
                return 0xff;
            goto next_macro_char;

        }
        else if (ch == '0') {    /* A HEX VALUE    */

         /*    FIRST CHARACTER MUST BE VALID HEX    */
            if ((ival = Hex_value(Macro_byte())) == not_hex) {
                Macro_file_error(bad_hex_value);
                macro_fail = _TRUE;
                return 0xff;
            }
            /*    SECOND MAY BE HEX OR NOT HEX. BACKUP IF NOT    */
            if ((ch = Hex_value(Macro_byte())) == not_hex)
                macro_next--;
            else
                ival = (ival << 4) + ch;
            return ival;
        }
        else {
            /* OR IS \CU, \CH ETC. */
/*    SET NAME_WORD TO THE NEXT TWO CHARACTERS    */
            namepair = CHAR2(Upper(ch), Upper(Macro_byte()));

            /*    SEARCH NAMES OF INPUT CODES    */
            for (i = 0; i <= last(input_code_names); i++) {
                if (input_code_names[i] == namepair)
                    return first_code + i;
            }
            switch (namepair) {
            case CHAR2('T', 'B'): return TAB;
            case CHAR2('N', 'L'): return CR;
            case CHAR2('E', 'M'): found_em = 1; return 0;   // end of macro definition
            case CHAR2('M', 'M'): found_em = _TRUE; return 0;   // end of modeless macro definition
            }
            Macro_file_error("\xc" "bad '\\' code");
            macro_fail = _TRUE;
            ch = 0xff;
        }
    }
    return ch;
} /* macro_char */




/* RETURN THE NEXT NON-BLANK MACRO_BYTE.
   CHANGE CR TO ';' AND LOWER TO UPPER CASE. */

static byte Macro_not_blank() {
    byte ch;

read_next:
    ch = Macro_byte();
    if (ch == ' ' || ch == CR || ch == TAB) goto read_next;
    if (ch == '\\') {            /* comment */
        Process_macro_comment();
        if (macro_fail)
            return 0xff;
        goto read_next;
    }
    if (ch == LF)
        ch = ';';     /* CHANGE LF TO ';' */

    return Upper(ch);
} /* macro_not_blank */




/*
    NOT_EQUAL                        TRIVIAL ROUTINE TO SEE IF '=' IS THE
                                    NEXT MACRO CHARACTER.
*/

static byte Not_equal() {
    byte ch;

    ch = Macro_not_blank();
    if (ch == '=')
        return _FALSE;
    Macro_file_error("\xb" "missing '='");
    return _TRUE;
} /* not_equal */



/*
    NOT_SEMI_COLON                    TRIVIAL ROUTINE TO SEE IF ';' IS THE
                                    NEXT MACRO CHARACTER.
*/

static byte Not_semi_colon() {
    byte ch;

    ch = Macro_not_blank();
    if (ch == ';')
        return _FALSE;
    Macro_file_error("xb" "missing ';'");
    return _TRUE;
} /* not_semi_colon */



/*
    MACRO_NUMBER                    INPUT A BYTE NUMBER FROM MACROFILE.
*/

static word Macro_number() {
    byte ch; dword num;

    num = 0;

    while ((ch = (Macro_not_blank() - '0')) < 10) {
        num = num * 10 + ch;
    }
    macro_next--;; /* we read one char too far; backup */
    if (num > 0xffff)
        num = 0;
    return num;
} /* macro_number */



/*
    BAD_HEX                            INPUTS A ZERO TO FOUR CHARACTER HEX
                                    VALUE INTO SET_AT. IF ERROR FOUND,
                                    RETURNS TRUE AND PRINTS MESSAGE.
*/

static byte Bad_hex(pointer code_p,     // destination of hex string
    byte max_num,       // max length of the input number (in bytes,not digits)
    boolean in_place) { // store the string at *code_p even if it is long


    byte str[81];
    byte ch;
    byte i, j;

    if (Not_equal() == _FALSE) {    /* NEED = FIRST    */
        str[0] = 0;            /* ACCUMULATE CHARACTERS IN str */
        while (Hex_value(ch = Macro_not_blank()) != not_hex && str[0] < 80) {
            str[++str[0]] = Hex_value(ch);
        }
        if (ch == ';') {        /* NEED ; AT END OF HEX    */
            if (str[0] == 0) {  /* NULL MEANS FUNCTION NOT AVAILABLE */
                code_p[0] = 0;
                return _FALSE;
            }
            if (str[0] == 1) {    /* SINGLE CHAR IS OK    */
                code_p[0] = 1;
                code_p[1] = str[1];
                return _FALSE;
            }
            /* else must be even */
            if (str[0] <= max_num * 2 && ((str[0] & 1) == 0)) {
                if (str[0] <= 8 || in_place) {
                    code_p[0] = str[0] / 2; /* get the length */
                    i = 0;
                    while (i < str[0]) {
                        code_p++;
                        code_p[0] = str[++i] << 4;
                        code_p[0] += str[++i];
                    }
                    return _FALSE;
                }
                else {
                    i = 0;   j = 0;
                    while (i < str[0]) {
                        j++;
                        str[j] = str[++i] << 4;
                        str[j] = code_p[0] + str[++i];      // possible buf should code_p[0] be str[j]
                    }
                    str[0] = str[0] / 2;
                    Insert_long_config(str, (entry_t *)code_p);
                    return _FALSE;
                }
            }
        }
    }

    Macro_file_error(bad_hex_value);
    return _TRUE;
} /* bad_hex */




/*
    ADD_MACRO_CHAR                        ADD ONE CHARACTER TO CURRENT MACRO
                                        DEFINITION. PRINT APPROPRIATE
                                        MESSAGE IF NO ROOM LEFT.
*/

boolean Add_macro_char(byte ch) {

    if (new_macro->text + new_macro->text_length >= &macros[macro_buf_size]) {
        if (in_macro_def) {
            Error(no_more_room);
            ms_sleep(3000);
        }
        else {
            Macro_file_error(no_more_room);
        }
        in_macro_def = _FALSE;
        return _FALSE;
    }
    new_macro->text[new_macro->text_length++] = ch; /* ADD BYTE TO MACRO TEXT */
    return _TRUE;
} /* add_macro_char */





/*
    FIND_MACRO                            SEES IF STRING IN INPUT_BUFFER IS
                                        A MACRO NAME. IF TRUE, SETS MACRO_AT TO
                                        WORD CONTAINING LENGTH OF MACRO NAME.
                                        RETURNS TRUE IF MATCH FOUND. */

static byte Find_macro() {

    macro_at = macros;                    /* POINT TO FIRST ENTRY    */

    while (macro_at < macro_end) {        /* SEARCH BUFFER    */
        if (macro_name_length == input_buffer[0] &&
            /* MUST BE SAME LENGTH    */
            Cmp_name(input_buffer, macro_at))
            return _TRUE;
        /* AND BE THE SAME    */
        macro_at += macro_name_length + sizeof(byte);
        macro_at += macro_text_length + sizeof(word);
    }
    return _FALSE;
} /* find_macro */




/*
    CAN_ADD_MACRO                    DELETE OLD MACRO WITH SAME NAME AND
                                    SEE IF SOME ROOM REMAINS FOR ANOTHER
                                    MACRO. RETURN TRUE IF ALL IS OK.
*/

static byte Can_add_macro() {
    pointer old;

    if (Find_macro()) {        /* MUST DELETE OLD MACRO    */

        if (macro_exec_level != 0) {
            /*
               Redefinition  of an existing macro while executing a macro is
               forbidden.  If there is an old macro with the same name, then
               all the macros that reside in the macro buffer following that
               macro will move, including the active ones, and pointers will
               be no longer valid.
            */
            if (creating)
                Error(create_while_exec);
            else
                Macro_file_error(create_while_exec);
            return _FALSE;
        }

        old = macro_at;
        macro_at += macro_name_length + sizeof(byte);
        /* GO TO LENGTH OF MACRO TEXT */
        macro_at += macro_text_length + sizeof(word);
        /* POINTS TO FIRST AFTER OLD ONE */
        memcpy(old, macro_at, macro_end - macro_at);
        macro_end += old - macro_at;
    }

    if (macro_end + input_buffer[0] + 20 > macros + macro_buf_size) {
        if (creating)
            Error(no_more_room);
        else
            Macro_file_error(no_more_room);
        return _FALSE;
    }

    Move_name(input_buffer, macro_end);
    /* PUT NEW NAME IN MACROS    */
    /* MACRO_END IS LEFT AT THE WORD
        THAT SPECIFIES MACRO TEXT LENGTH */
    new_macro = (macrotext_t *)(macro_end + input_buffer[0] + 1);
    /* USE THIS TO DO THE ADD OF MACRO BYTES */
    new_macro->text_length = 0;        /* NO TEXT YET */
    return _TRUE;
} /* can_add_macro */





/* Find an index for a case statement from a char and a char list;
   will return then length of the list if the char is not found. */

byte Find_index(byte ch, pointer ch_list_p) {
    word i;

    if ((i = findb(&ch_list_p[1], ch, ch_list_p[0])) == 0xffff)
        return ch_list_p[0];
    else
        return (byte)i;
} /* find_index */





static void Process_t_or_f(pointer flag_p, pointer err_p) {
    byte i, ch;

    i = Not_equal();
    ch = Upper(Macro_not_blank());
    if (ch == 'T') {
        *flag_p = _TRUE;
    }
    else if (ch == 'F') {
        *flag_p = _FALSE;
    }
    else {
        Bad_value(err_p);
        return;
    }
    i = Not_semi_colon();

} /* process_t_or_f */

/*
 * Begin iRMX II.4 Fix, Part 6 of 8, 12/28/88
 * Feature addition allowing aedit to capture CLI terminal name and then read
 * :config:termcap for initial terminal configurations based on this name.
 */
 /*
  * End iRMX II.4 Fix, Part 6 of 8
  */


  /*
      MACRO_FILE_PROCEDURES                    DO THE MANUAL LABOR OF INPUTTING
                                          A MACRO FILE. CALLED FROM START CODE
                                          AS WELL AS FROM MACRO_GET. NAME IS
                                          ALREADY IN MACRO_NAME.

  */

void Macro_file_process() {
    byte blankout_code[5];
    byte ch; word num;
    wbyte i;

    macro_line = 1;                        /* FIRST LINE IS NUMBER 1 */
    current_prompt[0] = 0;

    /*
     * Begin iRMX II.4 Fix, Part 7 of 8, 12/28/88
     * Feature addition allowing aedit to capture CLI terminal name and then read
     * :config:termcap for initial terminal configurations based on this name.
     */
     /*
      * End iRMX II.4 Fix, Part 7 of 8
      */

      /*    IF NO NAME THEN THE PART OF CURRENT BUFFER THAT IS IN MEMORY IS
          START OF 'MACROFILE'. THIS IS
          'CHEATING' - SPILLED MACRO FILES ARE NOT HANDLED CORRECTLY. HOWEVER
          A SPILLED MACRO FILE IS DEGENERATE AND IS EITHER A MILLION BLANKS
          OR WILL OVERFLOW THE MACRO BUFFER ANYWAY. */

    if (null_macro_filename) {
        macro_next = oa.low_s;
        macro_last = oa.low_e;
    }
    else {        /*    MUST OPEN THE FILE    */
        macro_next = macro_last;    /* FORCE READ ON NEXT MACRO_CHAR CALL */
    }

    macro_fail = _FALSE;

next_macro_line:

    while (1) {
        ch = Macro_not_blank();
        if (macro_fail)
            return;

        switch (ch) {

        case 'A':	        /* Case A */

            ch = Upper(Macro_byte());
            switch (ch) {
            case 'V':	     /* FOUND A AV    */
                i = Not_equal();
                num = Macro_number();
                if (num < 5 || num > 66) {
                    Bad_value("AV");
                }
                else {
                    i = Not_semi_colon();
                    if (!window_present) {
                        window.prompt_line = prompt_line = num - 1;
                        window.message_line = message_line = num - 2;
                        window.last_text_line = last_text_line = num - 3;
                    }
                }

                break;

            case 'B':	   /* FOUND AN AB    */
                if (Bad_hex(input_codes[esc_code - first_code].code,
                    4, _FALSE));
                break;

            case 'O':	    /* FOUND AN AO    */
                if (Bad_hex(output_codes[offset_index].code, 4, _FALSE));

                break;

            case 'R':	    /* FOUND AN AR    */
                if (Bad_hex(input_codes[rubout_code - first_code].code, 4, _FALSE));

                break;

            case 'W':	        /* FOUND AN AW    */
                Process_t_or_f(&wrapper, "AW");

                break;

            case 'X':         /* FOUND AN AX    */
            {
                boolean flag;
                Process_t_or_f(&flag, "AX");
                if (flag)
                    first_coordinate = col_first;
                else
                    first_coordinate = row_first;
            }
            break;

            case 'F':	    /* FOUND A ALTER FUNCTION */
                    /*    SET NAME_WORD TO THE NEXT TWO CHARACTERS    */
                namepair = Upper(Macro_byte());
                namepair = CHAR2(namepair, Upper(Macro_byte()));
                /*    SEE IF IT IS THE BLANKOUT CHARACTER    */
                switch (namepair) {
                case CHAR2('B', 'K'):
                        if (!Bad_hex(blankout_code, 4, _FALSE)) {
                            /* THE BLANKOUT CHARACTER IS STORED IN PRINT_AS    */
                            if (blankout_code[0] == 1)
                                print_as[' '] = blankout_code[1];
                        }
                        break;
                case CHAR2('C','C'):

                        /* the ^C replacement character */
                        if (Bad_hex(cc_code_buf, 4, _FALSE))
                            ;
                        break;
                case CHAR2('S', 'T'):
                        if (!Bad_hex(input_buffer, 40, _TRUE)) {
                            if (!batch_mode) {
                                Print_unconditionally_p(input_buffer);
                                Co_flush();
                            }
                        }
                        break;
                case CHAR2('E', 'N'):
                        if (Bad_hex(exit_config_list, 40, _TRUE))
                            ;
                        break;
                default:
                        /*    SEE IF AF IS FOR AN INPUT FUNCTION    */
                    for (i = 0; i <= last(input_code_names); i++) {
                        if (input_code_names[i] == namepair) {
                            if (Bad_hex(input_codes[i].code, 4, _FALSE))
                                ;
                            goto next_macro_line;
                        }
                    }

                    /*    SEE IF AF IS FOR AN OUTPUT FUNCTION    */

                    for (i = 0; i <= last(output_code_names); i++) {
                        if (output_code_names[i] == namepair) {
                            if (Bad_hex(output_codes[i].code, 20, _FALSE))
                                ;
                            goto next_macro_line;
                        }
                    }

                    /*    MATCHES NOTHING - MUST BE ERROR. PRINT MESSAGE
                        AND KEEP GOING    */

                    Macro_file_error("\xb" "bad AF type");
                    break;
                }
                break;

            case 'D':
            {	    /* FOUND A ALTER DELAY TIME */
                /* SET NAME_WORD TO THE NEXT TWO CHARACTERS */
                word namepair = Upper(Macro_byte());
                namepair = CHAR2(namepair, Upper(Macro_byte()));
                if (namepair == CHAR2('C','O')) {
                    ch = Not_equal();
                    delay_after_each_char = Macro_number();
                    goto next_macro_line;
                }

                /* SEE IF AF IS FOR AN OUTPUT FUNCTION */
                for (i = 0; i <= last(output_code_names); i++) {
                    if (namepair == output_code_names[i]) {
                        ch = Not_equal();
                        delay_times[i] = Macro_number();
                        goto next_macro_line;
                    }
                }

                /* MATCHES NOTHING - MUST BE ERROR.
                   PRINT MESSAGE AND KEEP GOING */
                Bad_value("AD");
            }
                break;

            case 'I':	    // invisible attribute
                ch = ~visible_attributes;
                Process_t_or_f(&ch, "AI");
                visible_attributes = ~ch;

                break;

            case 'C':	// character_attribute
                Process_t_or_f(&character_attributes, "AC");
                break;

            case 'H': {       // hardware

                byte str[10];
                byte i, ch;
                i = Not_equal();
                Init_str(str, 10);
                ch = Macro_not_blank();
                while (ch != ';' && !macro_fail) {
                    Add_str_char(ch);
                    ch = Macro_not_blank();
                }
                if (str[0] == 0) {
                    Reset_config();
                }
                else if (Cmp_name(str, "\x2" "S3")) {
                    SIII_setup();
                }
                else if (Cmp_name(str, "\x3" "S3E")) {
                    SIIIE_setup();
                }
                else if (Cmp_name(str, "\x4" "S3ET")) {
                    SIIIET_setup();
                }
                else if (Cmp_name(str, "\x5" "PCDOS")) {
                    PCDOS_setup();
                }
                else if (Cmp_name(str, "\x2" "S4")) {
                    SIV_setup();
                }
                else if (Cmp_name(str, "\x5" "VT100")) {
                    VT100_setup();
                }
                else if (Cmp_name(str, "\x4" "ANSI")) {
                    ANSI_setup();
                }
                else {
                    Bad_value("AH");
                }
                break;
            }
            case 'S': // prompt
                Process_t_or_f(&input_expected_flag, "AS");
                if (input_expected_flag)
                    Set_input_expected('!');
                else
                    Set_input_expected('-');

                break;

            case 'T': // type ahead
                Process_t_or_f(&do_type_ahead, "AT");
                break;

            case 'Z':
                Process_t_or_f(&strip_parity, "AZ");
                break;

            case 'G':  /* Gong */
                Process_t_or_f(&bell_on, "AG");

                break;

            case 'M':   /* Max memory allocated by tmpman */
                i = Not_equal();
                max_tmpman_mem = Macro_number();

                break;

            default: // default
                Macro_file_error(bad_command);
                break;
                /* default_a_case */
            }
            // end case 0
            break;


        case 'S':   /* Case 'S' */
                /*    SET COMMANDS    */
            if (Set_from_macro() == _FALSE)
                ;
                break;


        case 'M': /* Case 'M' */
                /*    MACRO DEFINITIONS    */
            input_buffer[0] = 0;            /*    ACCUMULATE THE MACRO NAME    */
            ch = Upper(Macro_char());
            while (input_buffer[0] <= last(input_buffer) &&
                ch != esc_code && !macro_fail) {
                input_buffer[++input_buffer[0]] = ch;
                ch = Upper(Macro_char());
            }
            if (ch != esc_code || input_buffer[0] == 0) {
                Macro_file_error("\xd" "no macro name");
                break;
            }

            if (Can_add_macro()) {
                found_em = 0;            /* NOT \EM OR \AM YET */
                ch = Macro_char();
                while (found_em == 0 && !macro_fail) {
                    /* QUIT IF RUN OUT OF SPACE */
                    if (!Add_macro_char(ch)) {
                        macro_fail = _TRUE; /* abort macro file */
                        break;
                    }
                    /* GET A RAW CHARACTER BUT SKIP CR AND LF    */
                    ch = Macro_char();
                }
                if (macro_fail)
                    break;
                if (found_em == 0xff) {    /* MM, so add return char */
                    /* else was an AM, so we abort without
                       returning to previous state */
                    if (!Add_macro_char(0xff))
                        break;
                }
                macro_end = new_macro->text + new_macro->text_length;
            }
            break;


        case ';':    /* Case ';' */
                     /* IGNORE NULL COMMANDS */

                break;


        case 'D': // debugging not enabled
            ;

                break;


        default:  /* Default Case */
            Macro_file_error(bad_command);
                 break;
 
        }
    }
} /* macro_file_process */




/*
    STOP_MACRO                    DECREMENT COUNT OF CURRENTLY EXECUTING MACROS
                                IF ANY ARE EXECUTING. CALLED BY BACKUP_CHAR
                                ETC. WHENEVER AN ATTEMPT IS MADE TO MOVE OFF
                                THE WORLD. POINT TO SINGLE
                                ESCAPE CHARACTER TO STOP ANY MODE.
*/

void Stop_macro() {
    macrotext_t *temp;

    if (macro_exec_level > 0) {
        temp = macro_exec_pointer[macro_exec_level];
        if (temp->text[temp->text_length - 1] == 0xff)
            macro_exec_length[macro_exec_level] = temp->text_length - 1;
        else
            macro_exec_length[macro_exec_level] = temp->text_length;
        macro_exec_count[macro_exec_level] = 1;
        macro_exec_infinite[macro_exec_level] = _FALSE;
    }
} /* stop_macro */





/*
    MACRO_ADD_LEVEL                        EXECUTE THE MACRO WHOSE NAME IS
                                        POINTED TO BY MACRO_AT. THIS
                                        MEANS ADDING ANOTHER LEVEL TO THE
                                        EXECUTION STACK.
*/

static void Macro_add_level() {
    macrotext_t *temp;

    if (count == 0 && !infinite)
        return;

    if (macro_exec_level == last(macro_exec_pointer)) {
        Error("\x16" "macro nesting too deep");
        macro_exec_level = 0;        /* KILL ALL MACROS    */
        Rebuild_screen();
        return;
    }

    if (macro_exec_level == 0) {
        Working();
        Kill_message();
    }
    macro_exec_level++;
    temp = macro_exec_pointer[macro_exec_level] = (macrotext_t *)(macro_at + macro_name_length + sizeof(byte));
    next_char_is_escape = (temp->text[temp->text_length - 1] == 0xff);
    last_main_cmd = Upper(last_main_cmd);
    if (next_char_is_escape && last_main_cmd == 'B') {
        Error("\x2E" "modeless macros not supported in Block command");
        macro_exec_level = 0;        /* KILL ALL MACROS    */
        Rebuild_screen();
        return;
    }
    macro_exec_length[macro_exec_level] = 0;
    macro_exec_count[macro_exec_level] = count;
    macro_exec_infinite[macro_exec_level] = infinite;
    cnt_exe[macro_exec_level] = 1; /* first execution */
    cnt_exe[macro_exec_level + 1] = 0; /* the nested counters are no more valid.*/
    go_flag[macro_exec_level] = _FALSE;
    next_char_is_escape = (temp->text[temp->text_length - 1] == 0xff);
    if (last_main_cmd == 'I' || last_main_cmd == 'X')
        macro_end_state[macro_exec_level] = last_main_cmd;
    else
        macro_end_state[macro_exec_level] = esc_code;
} /* macro_add_level */



/*
    SINGLE_CHAR_MACRO                    Test to see if the single
                                        character passed is a macro,
                                        if it is, it adds it via
                                        macro_add_level and returns true,
                                        else return false
*/

logical Single_char_macro(byte ch) {

    input_buffer[0] = 1;        /* MAKE CHARACTER A STRING    */
    input_buffer[1] = Upper(ch);
    if (Find_macro()) {
        Macro_add_level();
        return _TRUE;
    }
    return _FALSE;
} /* single_char_macro */





void Exec_init_macro() {

    Move_name("\x4" "INIT", input_buffer);
    if (Find_macro()) {
        count = 1;
        infinite = _FALSE;
        Macro_add_level();
        do_print_banner = _TRUE; /* print the banner when you terminate the macro */
    }
    else {
        Print_banner();
        do_print_banner = _FALSE;
    }

} /* exec_init_macro */





void Handle_macro_exec_code() {

    byte ch;

    Print_message("\x7" "<MEXEC>");
    ch = Upper(Cmd_ci());
    Clear_message();
    if (!Single_char_macro(ch))
        Error(no_such_macro);

} /* handle_macro_exec_code */





/*
    INPUT_MACRO_NAME                    INPUT NAME OF A MACRO WHICH IS
                                        TO BE CREATED. RETURN TRUE IF NAME
                                        IS OK.
*/

static byte Input_macro_name() {
    wbyte i;

    if (Input_filename("\xc" "Macro name: ", s_macro_name) == CONTROLC ||
        input_buffer[0] == 0)
        return _FALSE;

    /*    FORCE ALL MACRO NAMES TO UPPER CASE    */

    for (i = 1; i <= input_buffer[0]; i++)
        input_buffer[i] = Upper(input_buffer[i]);

    return _TRUE;
} /* input_macro_name */




/************************************************************************
  RETURN TRUE IF IN MACRO EXECUTION. IF NOT EXECUTING MACROS OR IF MACRO
  EXECUTION IS ABOUT TO CEASE, RETURN FALSE. THIS TEST IS NEEDED BY THE
  RE_VIEW CODE THAT SUPPRESSES OUTPUT FOR MACROS THAT WANDER OFF OF THE
  SCREEN.
************************************************************************/

byte In_macro_exec() {
    macrotext_t *temp;

    if (macro_exec_level > 0) {
        if (macro_exec_level == 1) {
            temp = macro_exec_pointer[1];

            /*    IF MACRO EXHAUSTED THEN RETURN FALSE */
            if (macro_exec_count[1] == 1) {
                if (temp->text_length == macro_exec_length[1]
                    || (temp->text_length == macro_exec_length[1] + 1 &&
                        temp->text[macro_exec_length[1]] == 0xff))
                    return _FALSE;
            }
        }

        return _TRUE;
    }
    return _FALSE;
} /* in_macro_exec */





/*
    SUPPLY_MACRO_CHAR                    RETURN A CHARACTER FROM A MACRO.
                                        IF CANNOT, RETURN WITH MACRO_EXEC_LEVEL
                                        EQUAL TO ZERO.
*/

byte Supply_macro_char() {

    word mcount, mlength;
    macrotext_t *temp;

    while (macro_exec_level > 0) {
        if (next_char_is_escape) {
            next_char_is_escape = _FALSE;
            return esc_code;
        }
        temp = macro_exec_pointer[macro_exec_level];
        mlength = macro_exec_length[macro_exec_level];
        /*    IF MACRO EXHAUSTED THEN EITHER DECREMENT COUNT OR GO TO NEXT LEVEL */

        if (temp->text_length != mlength) {
            byte _char;
            _char = temp->text[mlength];
            if (_char == 0xff && temp->text_length == mlength + 1) {
                if (state == 0) {
                    state = 1;
                    return esc_code;
                }
                else {
                    state = 0;
                    _char = macro_end_state[macro_exec_level];
                    macro_exec_length[macro_exec_level] = mlength + 1;
                }
            }
            macro_exec_length[macro_exec_level] = mlength + 1;
            return _char;
        }
        else {
            /* check for keys on a macro transition */
            Check_for_keys();
            mcount = macro_exec_count[macro_exec_level];
            macro_exec_length[macro_exec_level] = 0;
            if (mcount == 1) {
                macro_exec_level--;
                if (macro_exec_level == 0) {
                    Rebuild_screen();
                    if (do_print_banner) {
                        /* this happens only at the end of INIT macro */
                        Print_banner();
                        do_print_banner = _FALSE;
                    }
                }
            }
            else if (!macro_exec_infinite[macro_exec_level]) {
                macro_exec_count[macro_exec_level] = mcount - 1;
                cnt_exe[macro_exec_level]++;
                go_flag[macro_exec_level] = _FALSE;
            }
        }
    }
    return 0;
} /* supply_macro_char */





/*
    MACRO_CREATE                        ASK FOR NAME. SET IN_MACRO TRUE SO
                                        THAT SUBSEQUENT CONSOLE INPUT IS
                                        TRAPPED.
*/

static void Macro_create() {
    if (Input_macro_name()) {
        creating = _TRUE; /* creating interactively. */
        if (Can_add_macro()) {
            in_macro_def = _TRUE;                /* FLAG TO TRAP SUBSEQUENT INPUT */
            Kill_message();            /* FORCE NEW MESSAGE OUT    */
        }
        creating = _FALSE;
    }
} /* macro_create */





/*
    MACRO_GET                            THE G SUBCOMMAND OF MACRO. PROMPTS
                                        FOR A MACRO FILE NAME.
                                        MACRO_FILE_PROCEDURES IS CALLED TO
                                        DO THE REAL WORK.
*/

static void Macro_get() {

    if (Input_filename("\xc" "Macro file: ", s_macro_file) != CONTROLC) {
        if (input_buffer[0] == 0) {
            null_macro_filename = _TRUE;
            Working();
        }
        else {
            null_macro_filename = _FALSE;
            Openi(mac_file, 2);
            Echeck();
            if (excep != 0)
                return;
        }
        Macro_file_process();
        /* MUST  DETACH THE MACRO FILE (IF ANY)    */
        if (!null_macro_filename)
            Detach(mac_file);
    }
} /* macro_get */




/*
    ADD_TWO                                ADD '\' AND TWO CHARACTERS TO TEXT.
*/

static void Add_two(word two) {

    Insert_a_char('\\');
    Insert_a_char(two >> 8);        // inverse of CHAR2
    Insert_a_char(two & 0xff);
} /* add_two */




/*
    WRAP_MACRO                            THE MACRO NEEDS TO BE WRAPPED TO NEXT
                                        LINE. INSERT A CR,LF AND RE_VIEW THE
                                        RESULT.
*/

static void Wrap_macro() {
    Save_line(row);
    Insert_crlf();
    Test_crlf();
    Re_view();
} /* wrap_macro */




/*
    INSERT_CHAR                            INSERT ONE CHARACTER INTO THE TEXT IN
                                        EXPANDED FORM - \CU, \0A ETC. RE_VIEW
                                        ON EVERY CHARACTER.
*/

static void Insert_char(byte ch) {

    if (col > 74 || ch == CR) {        /* AUTOMATICALLY WRAP THE LINE    */
        Wrap_macro();
        if (ch == CR)
            return;
    }

    /*    ACTUALLY ADD CHARACTERS TO TEXT HERE    */

    Save_line(row);
    if (ch == LF) {
        Add_two(CHAR2('0', 'A'));
    }
    else if (ch == TAB) {
        Add_two(CHAR2('T','B'));
    }
    else if (Is_illegal(ch)) {
        if (ch >= first_code && ch <= last_code) {
            Add_two(input_code_names[ch - first_code]);
        }
        else {                /* MUST OUTPUT IN HEX FORM    */
            Insert_a_char('\\');
            Insert_a_char('0');
            Insert_a_char(hex_digits[ch >> 4]);
            Insert_a_char(hex_digits[ch & 0xf]);
        }
    }
    else {
        Insert_a_char(ch);        /* NORMAL PRINTING CHARACTER    */
    }

    if (ch == '\\' && !dont_double_backslash)
        Insert_a_char('\\');    /* need double for backslash */
    dont_double_backslash = _FALSE;

    Re_view();
} /* insert_char */




/*
    INSERT_STRING                            CALLS INSERT CHAR FOR EVERY
                                            CHARACTER IN A STRING.
*/

static void Insert_string(pointer str) {

    byte i;

    i = *str;                                /* LENGTH OF STRING    */
    if (i + col > 74)
        Wrap_macro();        /* DONT MAKE THE LINE TOO LONG */
    while (i-- > 0)
        Insert_char(*++str);
} /* insert_string */




/*
    MACRO_INSERT                        INSERT CHARACTERS (IN EXPANDED
                                        FORM - \CU,\CH ETC.) IN TEXT.
                                        CONTINUE UNTIL USER HITS CONTROL C.
*/

static void Macro_insert() {
    byte ch;

    Set_dirty();
    while (1) {
        Print_prompt_and_repos("\x12" RVID "Control C to stop");
        if ((ch = Cmd_ci()) == CONTROLC)
            return;
        dont_double_backslash = _TRUE;
        Insert_char(ch);
    }
} /* macro_insert */




/*
    MACRO_LIST                        LIST THE NAMES OF THE MACROS ON THE
                                    MESSAGE LINE.
*/

static void Macro_list() {

    byte macros_str[] = { "\x09" " Macros: " };
    byte message_len = config == SIV ? 40 : 60; /* length of message line w/o 'other' , 'forward' */

    /*!!!    CALL need_screen;   TURN OFF MACRO_SUPPRESS IF ON */
    force_writing = _TRUE;
    macro_at = macros;                /* POINTER TO NEXT MACRO    */
    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_str(macros_str);
    while (macro_at < macro_end) {
        if (tmp_str[0] + macro_name_length >= message_len && tmp_str[0] != 9) {
            /* 9 is the length of macros_str */
            Print_message(tmp_str);
            if (Hit_space() != ' ') {
                force_writing = _FALSE;
                return;
            }
            Init_str(tmp_str, sizeof(tmp_str));
            Add_str_str(macros_str);
        }
        Add_str_special(macro_at);
        Add_str_char(' ');
        macro_at += macro_name_length + sizeof(byte);
        macro_at += macro_text_length + sizeof(word);
    }
    Print_message(tmp_str);
    if (macro_exec_level != 0)
        Co_flush();
    force_writing = _FALSE;
} /* macro_list */



/*
    MACRO_NULL                        DO NOTHING. THIS LETS THE USER OUT OF
                                    THE MACRO COMMAND WITH A CONTROLC OR ESC.
*/

static void Macro_null() {
    ;
} /* macro_null */




/*
    MACRO_SAVE                        SAVE A MACRO INTO THE TEXT IN FUNNY FORM.
*/

static void Macro_save() {

    byte ch;
    word index;
    macrotext_t *temp;
    logical end_type;

    if (Input_macro_name()) {
        if (Find_macro()) {
            Set_dirty();
            Working();
            Position_in_text();
            Insert_char('M');
            Insert_string(macro_at);
            Insert_char(esc_code);
            temp = (macrotext_t *)(macro_at + macro_name_length + sizeof(byte));
            index = 0;
            end_type = 0;

            while (index < temp->text_length && cc_flag == _FALSE) {
                ch = temp->text[index];
                if (ch == CR) {
                    dont_double_backslash = _TRUE;
                    Insert_string("\x3" "\\NL");
                }
                else if (ch == 0xff && index + 1 == temp->text_length)
                    end_type = 1;
                else
                    Insert_char(ch);
                index++;
            }
            dont_double_backslash = _TRUE;
            if (end_type == 0)
                Insert_string("\x3" "\\EM");
            else
                Insert_string("\x3" "\\MM");
            Insert_char(CR);
        }

        else Error(no_such_macro);
    }
} /* macro_save */




/*
    E_CMND                                INPUT A MACRO NAME AND EXECUTE THE
                                        POOR THING.
*/

void E_cmnd() {

    if (Input_macro_name()) {
        if (Find_macro())
            Macro_add_level();
        else
            Error(no_such_macro);
    }
} /* e_cmnd */




/*
    M_CMND                            M COMMAND. CREATE, EXECUTE LIST ETC MACROS.
                                    ALSO USED TO END MACRO CREATE MODE.
*/

void M_cmnd() {
    byte ch;

    if (in_macro_def) {        /* END MACRO DEFINITION    */
        /* MOVE MACRO_END PAST END OF NEW MACRO    */
        /* NOTE - DO NOT WANT M AT END OF MACRO    */
        new_macro->text_length--;
        in_macro_def = Add_macro_char(0xff);    /* a macro inserted from the
                                            keyboard by default restores state,
                                            only macros in the macro file
                                            can end with an Abort Macro (\EM)
                                            and not restore state
                                        */
        macro_end = new_macro->text + new_macro->text_length;
        in_macro_def = _FALSE;
        Kill_message();            /* FORCE NEW MESSAGE OUT    */

    }
    else {
        while (1) {
            ch = Input_command("\x4f"
                RVID "Create    Get       Insert    List     "
                RVID "Save                                  ");
            if (oa.file_disposition == view_only)
                if (ch == 'I' || ch == 'S')
                    ch = 0; /* illegal_command */
            if (Dispatched(ch, m_dispatch))
                return;
            Illegal_command();
        }
    }
} /* m_cmnd */

