// #SMALL
// #title('START                    INIT. AND QUIT ROUTINES')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

/* Modified  4/24/89 Douglas Jackson
 * I modified the PPR 2298 fix.  It wasn't working right.  We need to
 * set the access rights on the file IFF we are renaming to .bak.
 * I tested with rmx server, xenix server... no problems.
 * Find my initials: dhj
 */
#include <stdlib.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"
#include <memory.h>
#include <io.h>
#include <signal.h>


byte memory[memory_leng];


/*
 * Begin iRMX II.4 Fix, Part 1 of 8, 12/28/88
 * Feature addition allowing aedit to capture CLI terminal name and then read
 * :config:termcap for initial terminal configurations based on this name.
 */
 /*
  * End iRMX II.4 Fix, Part 1 of 8
  */

word memory_size;
word other_buffer_size;
word block_size;

word new_macro_buf_size = { 0 };

logical recovery_mode = { _FALSE };

/*    VARIABLES USED IN INITIALIALIZATION    */

byte first_o_command = { _TRUE };
/* TRUE FOR USERS FIRST O COMMAND */


byte s_init[string_len_plus_1] = { 0 };

boolean explicit_macro_file = { _FALSE };


// #eject


/**************************************************************************/
/*                                                                        */
/* THE FOLLOWING  ARE THE  ROUTINES THAT  ADD AND  REMOVE THE  EOF MARKER */
/* CHARACTER                                                              */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/*                            add_eof                                     */
/* Add "|" to end of buffer.                                              */
/*                                                                        */
/**************************************************************************/
void Add_eof() {

    if (oa.have_eof == _FALSE) {
        oa.have_eof = _TRUE;
        *oa.high_e++ = '|';
        *oa.high_e = LF;
    }
} /* add_eof */



/**************************************************************************/
/*                            subtract_eof                                */
/*  REMOVE THE | AT END OF BUFFER                                         */
/*                                                                        */
/**************************************************************************/
void Subtract_eof() {
    if (oa.have_eof) {
        oa.have_eof = _FALSE;
        *--oa.high_e = LF;
    }
} /* subtract_eof */

// #EJECT



/**************************************************************************/
/*                            print_name                                  */
/*                                                                        */
/* PRINT_NAME FORCE THE INPUT BUFFER  OUT  TO  ENCOURAGE  THE  USER  INTO */
/* THINKING THAT SOMETHING IS HAPPENING.                                  */
/*                                                                        */
/**************************************************************************/
static void Print_name(pointer name_p) {
    Print_message(name_p);
    Co_flush();
} /* print_name */


logical first_time = { _TRUE };

/**************************************************************************/
/*                            print_banner                                */
/*                                                                        */
/*                                                                        */
/**************************************************************************/
void Build_banner() {
    Init_str(tmp_str, sizeof(tmp_str));
    dq_get_system_id(tmp_str, &excep);
    if (first_time == _FALSE) {
        Add_str_char(' ');
    }
    Add_str_str(title);
    Add_str_str(version);
    if (first_time == _TRUE) {
        Add_str_str(copy_right);
        first_time = _FALSE;
    }
} /* build_banner */



void Print_banner() {
    Build_banner();
    Print_name(tmp_str);
} /* print_banner */





/**************************************************************************/
/*                            setup_memory                                */
/*                                                                        */
/* SEE HOW MUCH MEMORY WE HAVE TO PLAY WITH.  Only done during setup.     */
/*                                                                        */
/**************************************************************************/
static void Setup_memory() {


    byte insuf_mem[] = { "\x13" "insufficient memory" };
    byte ratio;

    /* allocate the macro buffer */
    if (new_macro_buf_size != 0) macro_buf_size = new_macro_buf_size;

    oa.low_s = &memory[macro_buf_size + 10];  /* FIRST FREE BYTE    */
    *oa.low_s++ = LF;   /* DEPOSIT LF AT START OF TEXT */

    oa.high_e = memory + sizeof(memory) - 4;

    /*
            To improve  performance, block_size must  be as large
            as possible.  This prevents a lot of memory movements
            by movmem when we spill text to temporary files as we
            walk  through the file.   The maximal block_size must
            be less  than one  third of  the smaller  between the
            main  text buffer and the  other text buffer (because
            the  maximum gap is block_size  minus epsilon, and if
            the  gap is at the middle  of the text_buffer, one of
            the memory  sections on its  sides must be  as big as
            block_size  to allow  spilling).   This leads  to the
            conclusion  that we must allocate  half of the memory
            for each file (main and other).

            On the other  hand, we  want to  make more  than half
            memory  available for users who don't have memory for
            virtual files, to make them able to edit larger files
            in the main  text  buffer  without  spilling,  so  we
            allocate two  thirds for the main,  and one third for
            the other.

            We devide by two if there is more than 16K memory for
            virtual files, and by three if there is not.

            Block_size should not be  smaller  than  2K,  because
            block_buffer's  size is  2K and  check_window assumes
            that    the     text     size     is     at     least
            block_buffer_size*3+K/2.
    */

    ratio = 2;


    memory_size = (word)(oa.high_e - oa.low_s);
    other_buffer_size = memory_size / ratio;
    block_size = ((other_buffer_size - 1024) / 3) & 0xfe00;

    if (oa.low_s >= oa.high_e || block_size < 2048)
        Early_error(insuf_mem);

    /*++++++++ OLD CODE
       block_size=((oa.high_e-oa.low_s-8*1024)/4) AND 0fe00h;
       IF block_size < 1024 THEN block_size=1024;
       IF block_size > 8*1024 THEN block_size=8*1024;
   |* BLOCKSIZE IS (TOTAL SIZE - 8K) / 4 ROUNDED DOWN TO A 512 BYTE MULTIPLE *|
     ++++++ */

} /* setup_memory */



/**************************************************************************/
/*                            clear_text                                  */
/*                                                                        */
/* THROW AWAY ALL TEXT.                                                   */
/*                                                                        */
/**************************************************************************/
static void Clear_text() {
    word save_tag;

    save_tag = oa.tblock[ed_tagw];        /* this should not be initialized */
    memset(oa.tblock, 0xff, sizeof(oa.tblock));     // REMOVE ALL TAGS

    /* if we have initialized the buffer that the other window is pointing
       on then set the cursor position in that window to the beginning of
       the file then leave the value of tblock initialized; otherwise,
       restore it */
    if (w_in_other != in_other)
        oa.tblock[ed_tagw] = save_tag;
    oa.low_e = oa.low_s;
    oa.bol = 0;
    /*PUT HIGH_E BACK TO STARTING LOCATION */
    if (oa.have_eof)
        oa.high_e--;
    cursor = oa.high_s = oa.high_e;

    Set_dirty();
    oa.dirty = oa.have_eof = _FALSE;
    Add_eof();                    /* ADD EOF MARK */
} /* clear_text */



/**************************************************************************/
/*                            read_input_file                             */
/*                                                                        */
/* READ THE INPUT FILE (IF ANY) INTO TEXT AREA.                           */
/*                                                                        */
/**************************************************************************/
static void Read_input_file() {

    word numin;

    Clear_text();                    /* WIPE OUT TEXT CONTENTS */

    if (files[in_file].conn != NULL) {
        Detach(in_file);
        oa.more_input = _FALSE;
    }
    Reinit_temp(oa.wk1_conn);
    Reinit_temp(oa.wk2_conn);
    oa.wk1_blocks = 0;
    oa.wk2_blocks = 0;
    oa.block_size = block_size;

    if (oa.input_name[0] > 0) {
        Openi(in_file, 2);            /* OPEN THE INPUT FILE */
        if (excep == 0) {
            if (oa.file_disposition == view_only) {
                Set_temp_viewonly(oa.wk1_conn, view_only_wk1,
                    files[in_file].conn);
                Set_temp_viewonly(oa.wk2_conn, view_only_wk2,
                    files[in_file].conn);
            }
            oa.new_file = _FALSE;
            oa.more_input = _TRUE;
            Subtract_eof();
            if (recovery_mode == _FALSE) {    /* read in the input file */
                while (oa.more_input &&
                    (oa.high_s - oa.low_e) > oa.block_size + window_minimum) {
                    if ((numin = Read(in_file)) != oa.block_size) {
                        Detach_input();
                        Add_eof();
                    }
                }
                cursor = oa.low_s;
                Re_window();
            }
        }
        else if ((excep == ENOENT && oa.output_name[0] == 0))
            excep = 0;
        Echeck();
        if (excep != 0)
            oa.input_name[0] = 0; /* KILL BAD INPUT FILENAME */

    /*    MAY HAVE TO WARN ABOUT FILE TOO BIG IF IN OTHER FILE */

        if ((oa.file_disposition == lose_file) && (oa.more_input)) {
            Error("\x11" "text does not fit");
            excep = 1;                        /* PRESERVES MESSAGE */
            oa.file_disposition = lost_file;
        }
    }

    if (oa.file_disposition == lose_file)
        oa.block_size = Min(block_size, 1024);

    if (recovery_mode == _TRUE) {
        recovery_mode = _FALSE;
        oa.high_e = oa.high_e - 2;
        oa.have_eof = _FALSE;
        cursor = oa.high_s = oa.low_e = oa.low_s;
        Add_eof();
        Check_window(oa.block_size - window_minimum);
        Re_window();
        return;
    }

} /* read_input_file */

/*
 * Begin iRMX II.4 Fix, Part 2 of 8, 12/28/88
 * Feature addition allowing aedit to capture CLI terminal name and then read
 * :config:termcap for initial terminal configurations based on this name.
 */
 /*
  * End iRMX II.4 Fix, Part 2 of 8
  */


  /****************************************************************************
      If the control  nomacro haven't been  specified then opens  and reads the
  macro file.   If the  control macro have  been specified then  opens the file
  name  taken from the control.  Else tries to open the default macro file.  In
  SIII the default  macro file  has the  same pathname  as the  invoked editor,
  except that its extension is .MAC .   In  RMX  and  XENIX?    there  are  two
  defaults:  the first is :HOME:aedit.mac in  RMX and $HOME/aedit.mac in XENIX.
  The second is exactly as the SIII default.
  ****************************************************************************/
static void Do_macro_file() {

    byte t_str[string_len_plus_1];

    Clear_text();
    /* SET UP TEXT POINTERS AS MACROS READ INTO WINDOW. */
    if (s_macro_file[0] != 0) {
        if (explicit_macro_file) {
            Openi(mac_file, 2);    /* TRY TO OPEN MACRO FILE    */
            // Echeck();           // openi already does echeck
        }
        else {
            Move_name(s_macro_file, t_str);
            Init_str(s_macro_file, string_len_plus_1);
#ifdef MSDOS
            Add_str_str(Getenv("USERPROFILE"));
#else
            Add_str_str(Getenv("HOME"));
#endif
            if (s_macro_file[0] != 0)
                Add_str_char('/');
            Add_str_str("\x9" "aedit.mac");

            Openi(mac_file, 2);    /* TRY TO OPEN DEFAULT MACRO FILE    */
            if (excep != 0) {
                Move_name(t_str, s_macro_file);
                Openi(mac_file, 2);    /* TRY TO OPEN DEFAULT MACRO FILE    */
            }
        }
        if (excep == 0) {
            Macro_file_process();
            Detach(mac_file);
        }
    }
} /* do_macro_file */











/**********************************************************
*              invocation-line parsing                    *
***********************************************************/



boolean in_invocation_line;    /* we need it to know if the error is fatal */
boolean was_error; /* we need it only in q_cmnd because parsing errors
                          are not fatal there. */


byte illegal_invocation[] = { "\x12" "illegal invocation" };
byte mac_buf_too_large[] = { "\x16" "macro buffer too large" };
byte mac_buf_too_small[] = { "\x16" "macro buffer too small" };
byte conflict[] = { "\x14" "conflicting controls" };



static void Start_error(pointer msg) {

    byte str[81];
    was_error = _TRUE;
    oa.input_name[0] = 0;
    oa.output_name[0] = 0;
    oa.file_disposition = keep_file;
    Init_str(str, sizeof(str));
    Add_str_str(msg);
    Add_str_str("\x9" ", NEAR: \"");
    Add_str_special(tmp_str); /* last token */
    Add_str_char('"');
    if (in_invocation_line)
        Early_error(str); /* fatal error - doesn't return */
    else
        Error(str);       /* non fatal error - returns    */
} /* start_error */




static void Invocation_error() {
    Start_error(illegal_invocation);
} /* invocation_error */


static void Conflict_error() {
    Start_error(conflict);
} /* conflict_error */



static byte Cmp_name_insensitively(pointer fromp, pointer top) {

    pointer limit;

    /* check for length first for fastest failure response */
    if (fromp[0] != top[0])
        return _FALSE;
    /* set limit to start + length */
    limit = fromp[0] + fromp;
    for (fromp++; fromp <= limit; fromp++) {
        top++;
        if (Upper(fromp[0]) != Upper(top[0]))
            return _FALSE;
    }
    return _TRUE;
} /* cmp_name_insensitively */




#define non_ctl	0
#define ctl_vo	1
#define ctl_fo	2
#define ctl_mr	3
#define ctl_ba	4
#define ctl_rc	5
#define ctl_novo	6
#define ctl_nofo	7
#define ctl_nomr	8
#define ctl_noba	9
#define ctl_norc	10
#define ctl_bf	11
#define ctl_db	12
#define ctl_to	13
#define ctl_comma	14
#define ctl_illegal	15
#define ctl_lpar	16
#define ctl_rpar	17
#define ctl_eoln	18

/*
 * Begin iRMX II.4 Fix, Part 1 of 5, 12/30/88
 * Feature addition adding TERMINAL invocation controls.
 */
 /*
  * End iRMX II.4 Fix, Part 1 of 5
  */

struct {
    byte val;
    pointer tokstr;
} keywords[] = {
    ctl_to,    "\x2" "TO",
    ctl_vo,    "\x2" "VO",
    ctl_fo,    "\x2" "FO",
    ctl_mr,    "\x2" "MR",
    ctl_ba,    "\x2" "BA",
    ctl_rc,    "\x2" "RC",
    ctl_bf,    "\x2" "MS",
    ctl_db,    "\x4" "_DB_",
    ctl_nomr,  "\x4" "NOMR",
    ctl_vo,    "\x8" "VIEWONLY",
    ctl_fo,    "\xb" "FORWARDONLY",
    ctl_mr,    "\x5" "MACRO",
    ctl_nomr,  "\x7" "NOMACRO",
    ctl_ba,    "\x5" "BATCH",
    ctl_rc,    "\x7" "RECOVER",
    ctl_bf,    "\x9" "MACROSIZE",
    /*
     * Begin iRMX II.4 Fix, Part 2 of 5, 12/30/88
     * Feature addition adding TERMINAL invocation controls.
     */
     /*
      * End iRMX II.4 Fix, Part 2 of 5
      */
        ctl_novo,  "\x4" "NOVO",
        ctl_novo,  "\xa" "NOVIEWONLY",
        ctl_nofo,  "\x4" "NOFO",
        ctl_nofo,  "\xd" "NOFORWARDONLY",
        ctl_norc,  "\x4" "NORC",
        ctl_norc,  "\x9" "NORECOVER",
        ctl_noba,  "\x4" "NOBA",
        ctl_noba,  "\x7" "NOBATCH"
};

/**************************************************************************/
/*                            control_type                                */
/*                                                                        */
/*                                                                        */
/**************************************************************************/
static byte Control_type() {            // reworked to C code

    for (int i = 0; i <= last(keywords); i++) {
        if (Cmp_name_insensitively(tmp_str, keywords[i].tokstr))
            return keywords[i].val;
    }
    return non_ctl;
} /* control_type */





byte delim = { ' ' };     /* DELIMITER OF DQ$GET$ARGUMENT */
byte arg_type;
boolean at_eoln = { _FALSE };   /* found end of line. */



/**************************************************************************/
/*                            get_arg                                     */
/*                                                                        */
/* READ AN  ARGUMENT.  CHOP  OFF IN THE  UNLIKELY CASE THAT  THE LENGTH IS*/
/* OVER string_len.                                                       */
/*                                                                        */
/**************************************************************************/
static void Get_arg() {


    byte i;
    byte extension[3];

    if (delim == CR || delim == LF) {
        if (at_eoln) {
            Invocation_error();
            arg_type = ctl_illegal;
            return;
        }
        else {
            at_eoln = _TRUE;
            arg_type = ctl_eoln;
        }

    }
    else if (delim == ',') {
        delim = ' ';
        arg_type = ctl_comma;

    }
    else if (delim == '-') {
        /* We visit here twice. On the first time we return ctl_comma,
           to make the difference between comma and minus transparent
           to the caller. On the second time we handle the extension
           and return a file name. */
        if (arg_type != ctl_comma) {
            arg_type = ctl_comma;
            /* leave delim asis for next visit */
        }
        else {
            delim = dq_get_argument(tmp_str, &excep); /* read extension */
            if (tmp_str[0] > 3 || tmp_str[0] == 0) {
                Invocation_error();
                arg_type = ctl_illegal;
                return;
            }
            /* MOVE EXTENSION IN tmp_str ASIDE */
            for (i = 1; i <= 3; i++) { /* MAXIMUM EXTENSION IS 3 */
                if (i <= tmp_str[0])
                    extension[i - 1] = tmp_str[i];
                else
                    extension[i - 1] = ' '; /* ADD BLANKS TO END OF EXTENSION */
            }
            /* OTHER NAME IS SAME AS MAIN NAME WITH NEW EXTENSION */
            Move_name(oa.input_name, tmp_str);
            dq_change_extension(tmp_str, extension, &excep);
            delim = ' ';
            arg_type = non_ctl;
        }

    }
    else if (delim == '(') {
        delim = ' ';
        arg_type = ctl_lpar;

    }
    else if (delim == ')') {
        delim = ' ';
        arg_type = ctl_rpar;

    }
    else if (delim != ' ') {
        arg_type = ctl_illegal;

    }
    else {
        delim = dq_get_argument(tmp_str, &excep);
        if (tmp_str[0] == 0) {
            if (delim == CR || delim == LF) {
                /* this situation occurs if the last char
                   in the invocation line is ')' */
                at_eoln = _TRUE;
                arg_type = ctl_eoln;
            }
            else {
                Invocation_error();
                arg_type = ctl_illegal;
            }
        }
        else if (tmp_str[0] > string_len) {
            Invocation_error();
            arg_type = ctl_illegal;
        }
        else {
            arg_type = Control_type();
        }
        /* if arg_type is non_ctl then the string is in tmp_str */
    }
} /* get_arg */






boolean ctl_vo_specified;
boolean ctl_fo_specified;
boolean ctl_to_specified;
boolean ctl_mr_specified = { _FALSE };
boolean ctl_ba_specified = { _FALSE };
/*
 * Begin iRMX II.4 Fix, Part 3 of 5, 12/30/88
 * Feature addition adding TERMINAL invocation controls.
 */
boolean ctl_bf_specified = { _FALSE };

/*
 * End iRMX II.4 Fix, Part 3 of 5
 */



 /**************************************************************************/
 /*                            get_controls                                */
 /* PARSE:                                                                 */
 /*    [MACROSIZE(SIZE)] |                                                 */
 /*    [MACRO[(FILENAME)]] | [NOMACRO] |                                   */
 /*    [[NO]BATCH] |                                                       */
 /*    [TERMINAL[(TERMINALNAME)]] | [NOTERMINAL]                           */
 /*                                                                        */
 /**************************************************************************/
 /* get_controls:PROC; */
static void Get_controls() {


    byte i;
    byte ch;
    dword num;
    while (1) {
        if ((arg_type != ctl_mr) && (arg_type != ctl_nomr) &&
            (arg_type != ctl_db) && (arg_type != ctl_bf) &&
            (arg_type != ctl_ba) && (arg_type != ctl_noba)
            /*
             * Begin iRMX II.4 Fix, Part 4 of 5, 12/30/88
             * Feature addition adding TERMINAL invocation controls.
             */) return;
             /*
              * End iRMX II.4 Fix, Part 4 of 5
              */

              /* MACRO case */
        if (arg_type == ctl_nomr) {
            if (ctl_mr_specified) Conflict_error();
            ctl_mr_specified = _TRUE;
            s_macro_file[0] = 0;       /* DO NOT READ A MACRO FILE */
        }
        else if (arg_type == ctl_mr) {
            explicit_macro_file = _TRUE;
            if (ctl_mr_specified) Conflict_error();
            ctl_mr_specified = _TRUE;
            Get_arg();
            if (arg_type == ctl_lpar) {
                Get_arg();
                if (arg_type != non_ctl)
                    Invocation_error();
                Move_name(tmp_str, s_macro_file);
                Get_arg();
                if (arg_type != ctl_rpar)
                    Invocation_error();
            }
            else {
                goto dont_get_arg_again;
            }

            /* MACROSIZE case */
        }
        else if (arg_type == ctl_bf) {
            if (ctl_bf_specified)
                Conflict_error();
            ctl_bf_specified = _TRUE;
            /* changing macro buffer size. default is 2048.
               the new size is limited to the range 2048..32K  */
            Get_arg();
            if (arg_type != ctl_lpar)
                Invocation_error();
            Get_arg();
            num = 0;
            for (i = 1; i <= tmp_str[0]; i++) {
                ch = tmp_str[i];
                if (ch < '0' || ch > '9')
                    Invocation_error();
                num = num * 10 + ch - '0';
            }
            if (num > 0x7fff) {
                Early_error(mac_buf_too_large);
            }
            else if (num < 1024) {
                Early_error(mac_buf_too_small);
            }
            else {
                new_macro_buf_size = num;
            }
            Get_arg();
            if (arg_type != ctl_rpar)
                Invocation_error();

            /*
             * Begin iRMX II.4 Fix, Part 5 of 5, 12/30/88
             * Feature addition adding TERMINAL invocation controls.
             */
             /*
              * End iRMX II.4 Fix, Part 5 of 5
              */

              /* debug case */

              /* BATCH case */
        }
        else if (arg_type == ctl_ba || arg_type == ctl_noba) {
            if (ctl_ba_specified)
                Conflict_error();
            ctl_ba_specified = _TRUE;
            batch_mode = (arg_type == ctl_ba);
        }
        Get_arg();

    dont_get_arg_again:
        ;
    } /* forever */

} /* get_controls */
/* ENDPROC get_controls; */



/**************************************************************************/
/*                            get_input_file_name                         */
/*                                                                        */
/* PARSE [file [TO file | VO | FO]                                        */
/*                                                                        */
/**************************************************************************/
static void Get_input_file_name() {

    oa.new_file = _TRUE;
    oa.file_disposition = keep_file;
    oa.input_name[0] = 0;
    oa.output_name[0] = 0;
    ctl_vo_specified =
        ctl_fo_specified =
        ctl_to_specified = _FALSE;
    if (arg_type == non_ctl) {
        Move_name(tmp_str, oa.input_name);
        Get_arg();
        while ((arg_type == ctl_vo) || (arg_type == ctl_fo) || (arg_type == ctl_to) ||
            (arg_type == ctl_novo) || (arg_type == ctl_nofo)) {
            if (arg_type == ctl_to) { /* parsing [TO FILE] */
                if (oa.file_disposition == view_only || oa.file_disposition == lose_file)
                    Conflict_error();
                Get_arg();
                if (arg_type != non_ctl)
                    Invocation_error();
                Move_name(tmp_str, oa.output_name);
                /* "TO :BB:" is synonym to forward_only */
                if (Cmp_name(tmp_str, "\x4" ":BB:"))
                    oa.file_disposition = lose_file;
            }
            else if (arg_type == ctl_vo) {
                if (ctl_vo_specified || ctl_to_specified
                    || oa.file_disposition == lose_file)
                    Conflict_error();
                ctl_vo_specified = _TRUE;
                oa.file_disposition = view_only;
            }
            else if (arg_type == ctl_fo) {
                if (ctl_fo_specified || ctl_to_specified
                    || oa.file_disposition == view_only)
                    Conflict_error();
                ctl_fo_specified = _TRUE;
                oa.file_disposition = lose_file;
            }
            else if (arg_type == ctl_novo) {
                if (ctl_vo_specified)
                    Conflict_error();
                ctl_vo_specified = _TRUE;
            }
            else {  /* nofo */
                if (ctl_fo_specified)
                    Conflict_error();
                ctl_fo_specified = _TRUE;
            }
            Get_arg();
        }
    }
    /* VO and FO are not allowed with a null filename */
    if (oa.input_name[0] == 0) oa.file_disposition = keep_file;

} /* get_input_file_name */



//#ifndef MSDOS
/*****************************************************************
    The  entire XENIX invocation line  is converted to normal
    format  and put in inv_line.  dq$switch$buffer causes the
    next calls  to dq$get$argument to read  from there.  From
    now on, the usual  invocation  line  interpreter  can  be
    used.
    The converted string is buffered in the array MEMORY, as
    it has no use yet.
*****************************************************************/


static void Add_in_quot() {
    if (tmp_str[0] != 0) {
        Add_str_char('"');
        Add_str_str(tmp_str);
        Add_str_char('"');
    }
} /* add_in_quot */

static void Add_param() {
    if (delim != ' ')
        Invocation_error();
    delim = dq_get_argument(tmp_str, &excep);
    Add_str_char('(');
    Add_in_quot();
    Add_str_char(')');
    if (delim != '-')
        Add_str_char(delim);
} /* add_param */

#ifndef MSDOS
#define size_name_str   100
#define size_ctl_str    400
#define size_vo_fo_str  500
#define size_inv_line   1000

// the code below is flawed in that Init_str should only take byte max size and
// repeated calls set the max length for string checks in this case to the size % 256

static void Convert_xenix_format() {
    pointer name_str = memory;                      /* editor's name*/
    pointer ctl_str = name_str + size_name_str;     /* the controls*/
    pointer vo_fo_str = ctl_str + size_ctl_str;     /* vo or fo  */
    pointer inv_line = vo_fo_str + size_vo_fo_str;  /* converted inv. line */

    /* get editor's name */
    Init_str(name_str, size_name_str);
    delim = dq_get_argument(tmp_str, &excep);
    Add_str_str(tmp_str);
    if (delim == '-')
        Add_str_char(' ');
    else
        Add_str_char(delim);

    /* get the controls */
    Init_str(ctl_str, size_ctl_str);
    Init_str(vo_fo_str, size_vo_fo_str);
    while (delim == '-') {
        delim = dq_get_argument(tmp_str, &excep);
        if (tmp_str[0] == 0 && delim == '-')
            /* '--' terminates controls section */
            break;
        arg_type = Control_type();
        if (arg_type == ctl_novo || arg_type == ctl_vo ||
            arg_type == ctl_nofo || arg_type == ctl_fo) {
            Reuse_str(vo_fo_str, size_vo_fo_str);
        }
        else {
            Reuse_str(ctl_str, size_ctl_str);
        }
        Add_in_quot();
        Add_str_char(' ');
        if (delim != '-' && delim != CR && delim != LF)
            Add_str_char(delim);
        if (arg_type == ctl_mr) {
            if (delim != '-' && delim != CR && delim != LF)
                Add_param();
        }
        else if (arg_type == ctl_bf) {
            Add_param();
        }
        else if (arg_type != ctl_noba && arg_type != ctl_ba &&
            arg_type != ctl_novo && arg_type != ctl_vo &&
            arg_type != ctl_nofo && arg_type != ctl_fo &&
            arg_type != ctl_nomr) {
            Invocation_error(); /* fatal */
        }
    }

    //exloop:
        /* read the rest of the invocation line */
    Init_str(inv_line, size_inv_line);
    Add_str_str(name_str);                  /* editor's name */

    while (delim != CR && delim != LF) {
        delim = dq_get_argument(tmp_str, &excep); 
        if (delim == '-')
            Invocation_error();
        arg_type = Control_type();
        if (arg_type != non_ctl && arg_type != ctl_to)
            Invocation_error();
        Add_in_quot();
        if (delim == CR || delim == LF)
            Add_str_char(' ');
        else Add_str_char(delim);
    }

    /* append the controls to the invocation line */
    Add_str_str(vo_fo_str);
    Add_str_str(ctl_str);
    Add_str_char(CR);

    dq_switch_buffer(&inv_line[1], &excep);
    Echeck();

} /* convert_xenix_format */

#endif











/**************************************************************************/
/*                            parse_tail                                  */
/*                                                                        */
/* PARSE  THE COMMAND TAIL AND READ THE INPUT AND MACRO FILES.  Only done */
/* during startup.                                                        */
/*                                                                        */
/**************************************************************************/
static void Parse_tail() {

    in_invocation_line = _TRUE;
#ifndef MSDOS
    dq_special(5, "", &excep);      /* set case sensitivity */
    Echeck();
    Convert_xenix_format()
#endif
        delim = ' ';
    at_eoln = _FALSE;
    tmp_str[0] = 0;
    Get_arg();
    if (arg_type != non_ctl) Invocation_error(); /* fatal */
    /* USE THE INVOCATION NAME IN ORDER TO CALCULATE THE DEFAULT MACRO FILE NAME */
    Move_name(tmp_str, s_macro_file);
    dq_change_extension(s_macro_file, "mac", &excep);

    Get_arg();

    if (arg_type == non_ctl)
        Get_input_file_name();

    if ((arg_type == ctl_rc) || (arg_type == ctl_norc)) {
        if (arg_type == ctl_rc)
            recovery_mode = _TRUE;
        Get_arg();
    }

#ifdef MSDOS
    if (arg_type == ctl_comma) {
        Get_arg();
#else
    if (arg_type == non_ctl) {
#endif
        Flip_pointers();                /* SWITCH TO THE OTHER FILE  */
        Get_input_file_name();
        Flip_pointers();                /* BURY OTHER POINTERS AGAIN */
    }

    Get_controls();

    if (arg_type != ctl_eoln)
        Invocation_error();

} /* parse_tail */






/**************************************************************************/
/*                            alter_init                                  */
/*                                                                        */
/* THIS IS  THE 'ON STARTUP ONLY' PART OF  ALTER.  PARSE THE COMMAND TAIL */
/* AND SET UP EDITER Only done during startup.                            */
/*                                                                        */
/**************************************************************************/
void Alter_init() {

#define number_buffers	6
#define nK_buffer_space	12

#ifndef MSDOS
    Ignore_quit_signal();
    Handle_ioctl();
#endif
    input_expected_flag = _FALSE;
#ifdef MSDOS
    excep = signal(SIGINT, Cc_trap) == SIG_ERR ? errno : 0;     /* MUST TRAP CONTROL C */
#endif
    Echeck();
    Tmpman_init();   /* for the 286 */
    Open_cico();     /* OPEN :CI: AND :CO: */
    input_expected_flag = _TRUE;
    Setup_terminal();
    Parse_tail();


    Setup_memory();

    macro_suppress = batch_mode;
    /* in batch - suppress all output except messages */

    error_status.in_invocation = _TRUE;

    /*
     * Begin iRMX II.4 Fix, Part 3 of 8, 12/28/88
     * Feature addition allowing aedit to capture CLI terminal name and then read
     * :config:termcap for initial terminal configurations based on this name.
     */
     /*
      * End iRMX II.4 Fix, Part 3 of 8
      */


    Do_macro_file();  /* HANDLES THE MACRO FILE    */

    center_line = (prompt_line + 1) / 5;

    if (do_type_ahead == 1)   /* undefined, AT not specified in the macro file */
        do_type_ahead = _TRUE;

    Check_minimal_codes_set();

    Read_input_file();            /* ACTUALLY READ THE INPUT FILE */

    error_status.in_invocation = _FALSE;

    if (visible_attributes && character_attributes)
        Put_normal_video();

    Put_clear_all(); /* clear screen */


    Re_view();

    /* If there is a macro named INIT, execute it */
    Exec_init_macro();

} /* alter_init */





/**************************************************************************/
/*                            flip_pointers                               */
/*                                                                        */
/* EXCHANGE  THE POINTERS TO THE  CURRENT TEXT WITH THE  OTHER SET.  FLIP */
/* THE IN_OTHER FLAG.                                                     */
/**************************************************************************/
void Flip_pointers() {

    ob.tblock[ed_tagw] = oa.tblock[ed_tagw];
    ob.toff[ed_tagw] = oa.toff[ed_tagw];

    in_other = ~in_other;


    oa_struc temp;      // use C support for structure copy
    temp = oa;
    oa = ob;
    ob = temp;


    /*    THESE CONNECTIONS MUST ALSO BE SAVED    */

    ob.in_conn = input_conn;
    input_conn = oa.in_conn;

} /* flip_pointers */







/*    THE FOLLOWING IS ALL RELATED TO THE QUIT COMMAND    */

/**************************************************************************/
/*                            rename_as_bak                               */
/*                                                                        */
/* RENAME INPUT FILE TO FILE WITH  EXTENSION  OF  '.BAK'.    IF  FILE  IS */
/* ALREADY A .BAK THEN DO NOTHING.                                        */
/*                                                                        */
/**************************************************************************/
static void Rename_as_bak() {
    char fname[string_len_plus_1];
    // convert name to C string format
    memcpy(fname, oa.input_name + 1, oa.input_name[0]);
    fname[oa.input_name[0]] = 0;

    /* rename only if there is a write permission */

    if (_access(fname, 6) != 0)  /* no write permission */
        return;

    Move_name(oa.input_name, oa.output_name);
    dq_change_extension(oa.output_name, "bak", &excep);
    Echeck();
    if (Cmp_name(oa.input_name, oa.output_name) == _FALSE) {
        file_num = in_file;
        Working();
        dq_delete(oa.output_name, &excep);
        if (excep == 0 || excep == ENOENT)
            dq_rename(oa.input_name, oa.output_name, &excep);
        Echeck();
    }
    oa.output_name[0] = 0;                /* NO OUTPUT NAME */
} /* rename_as_bak */



/**************************************************************************/
/*                            copy_wk1_file                               */
/*                                                                        */
/* USED BY WRITE ENTIRE FILE ONLY.  WRITES WK1 FILE TO OUTPUT.            */
/*                                                                        */
/**************************************************************************/
static void Copy_wk1_file(byte outfile) {
    word blocks;

    blocks = oa.wk1_blocks;
    if (blocks > 0) {
        Rewind_temp(oa.wk1_conn);
        while (blocks-- > 0) {
            Read_temp(oa.wk1_conn, oa.low_e);
            if (excep != 0)
                break;
            Write(outfile, oa.low_e, oa.block_size);
            if (excep != 0)
                break;
        }
        //    ex_loop:
        Restore_temp_state(oa.wk1_conn);
    }
} /* copy_wk1_file */



/**************************************************************************/
/*                            copy_wk2_file                               */
/*                                                                        */
/* USED BY WRITE ENTIRE FILE ONLY.  WRITES WK2 FILE TO OUTPUT.            */
/*                                                                        */
/**************************************************************************/
static void Copy_wk2_file(byte outfile) {
    byte i;
    word blocks;

    blocks = oa.wk2_blocks;
    if (blocks > 0) {
        while (blocks-- > 0) {
            excep = 0;
            i = Read_previous_block(oa.wk2_conn, oa.low_e, _TRUE);
            if (excep != 0)
                break;;
            Write(outfile, oa.low_e, oa.block_size);
            if (excep != 0)
                break;
        }
        //    ex_loop:
        Skip_to_end(oa.wk2_conn);
    }
} /* copy_wk2_file */



/**************************************************************************/
/*                            write_entire_file                           */
/*                                                                        */
/* WRITE ALL OF TEXT TO THE SPECIFIED FILE                                */
/*                                                                        */
/**************************************************************************/
static void Write_entire_file(byte nfile) {
    word len_high;

    /*    FIRST FORCE ANY TEXT LEFT IN THE INPUT FILE INTO THE WORK FILES    */

    Openo(nfile, 2, 2);            /* CREATE THE OUTPUT FILE */
    if (excep != 0)
        return;

    /*    IF EITHER WORK FILE IS IN USE THEN WILL USE WINDOW AREA AS A BUFFER */

    if (oa.wk1_blocks + oa.wk2_blocks > 0)
        Check_window(oa.block_size - window_minimum);

    Copy_wk1_file(nfile); /* COPY WK1 FILE TO OUTPUT */
    if (excep == 0) {
        Write(nfile, oa.low_s, (word)(oa.low_e - oa.low_s)); /* COPY BELOW WINDOW */
        if (excep == 0) {
            len_high = (word)(oa.high_e - oa.high_s);
            if (oa.have_eof)
                len_high--;
            Write(nfile, oa.high_s, len_high); /* COPY ABOVE WINDOW */
            if (excep == 0)
                Copy_wk2_file(nfile);/* COPY WK2 FILE TO OUTPUT */
        }
    }
    //ex_write:
    Detach(nfile);

} /* write_entire_file */





/**************************************************************************/
/*                            display_filenames                           */
/*                                                                        */
/*  PRINT THE 'Editing file to file'  MESSAGE.                            */
/*                                                                        */
/**************************************************************************/
static void Display_filenames() {

    boolean save_force_writing;

    save_force_writing = force_writing;
    force_writing = _TRUE;
    if (oa.input_name[0] == 0) {
        Print_message("\xd" "no input file");
    }
    else {
        Init_str(tmp_str, sizeof(tmp_str));
        Add_str_str("\x8" "Editing ");
        Add_str_str(oa.input_name);
        if (oa.file_disposition == lost_file) {
            /* WARN THAT LOST BLOCKS */
            Add_str_str("\x14" " which has lost text");
        }
        else if (oa.output_name[0] > 0) {
            Add_str_str("\x4" " to ");
            Add_str_str(oa.output_name);
        }
        Print_message(tmp_str);
    }
    force_writing = save_force_writing;

} /* display_filenames */





/**************************************************************************/
/*                            q_cleanup                                   */
/*                                                                        */
/* QUIT IS DONE - GO BACK TO TAG I                                        */
/*                                                                        */
/**************************************************************************/
static void Q_cleanup() {

    Kill_message();                /* GET RID OF QUIT MESSAGE */
    Jump_tagi();                    /* GO BACK TO START    */
} /* q_cleanup */





/**************************************************************************/
/*                            quit_exit                                   */
/*                                                                        */
/* GO TO LAST LINE, CLEAR IT AND THEN CALL DQ$EXIT.                       */
/*                                                                        */
/**************************************************************************/
void Quit_exit(byte status) {

    byte lastl;

    Working();
    force_writing = _TRUE;
    /* CALCULATE LINE IN WHICH TO LEAVE CURSOR */
    lastl = Max(prompt_line, window.prompt_line);
    /* ON SERIES IV DO NOT LEAVE USER STUCK IN THE PROMPT LINE    */
    if (config == SIV)
        lastl = lastl - 2;
    Put_start_row(lastl);        /* GOTO FINAL LINE */
    Put_erase_entire_line();        /* WIPE IT OUT    */
    Put_start_row(lastl);        /* GOTO FINAL LINE */
    Restore_system_config();
    Co_flush();

    Close_ioc();
    exit(status);
} /* quit_exit */





/**************************************************************************/
/*                            keep_after_all                              */
/*                                                                        */
/* ASK FOR  CONFIRMATION OF QUIT  ABORT OR INIT  IF THE USER  IS ABOUT TO */
/* LOSE SOME CHANGES.                                                     */
/*                                                                        */
/**************************************************************************/
static byte Keep_after_all() {
    byte ans;

    Rebuild_screen();
    ans = Input_yes_no_from_console("\x11" "all changes lost?", _FALSE, _FALSE);
    if (ans == CONTROLC)
        return _TRUE;
    return ~ans;
} /* keep_after_all */



/**************************************************************************/
/*                            keep_other_after_all                        */
/*                                                                        */
/* CALLED  BEFORE EXIT  TO ENSURE  THAT USER  DOES NOT  ACCIDENTALLY LOSE */
/* CHANGES IN THE OTHER FILE.                                             */
/*                                                                        */
/**************************************************************************/
static byte Keep_other_after_all() {

    if (first_o_command == _FALSE) {
        Flip_pointers();        /* EXCHANGE THE BUFFER SPECIFIC INFO. */
        if (oa.dirty && oa.file_disposition != lost_file) {
            Display_filenames();
            Print_prompt("\x1" RVID);/* KILL PROMPT TO FORCE RE-WRITE */
            w_dirty = _TRUE; /* needed? */
            V_cmnd();
            Re_view();
            if (Keep_after_all()) return _TRUE;
        }
    }
    return _FALSE;
} /* keep_other_after_all */





/**************************************************************************/
/*                            o_cmnd                                      */
/*                                                                        */
/* THE OTHER COMMAND                                                      */
/*                                                                        */
/**************************************************************************/
void O_cmnd() {
    pointer start_at;

    if (first_o_command) {

        /* MUST SQUEEZE THE MAIN TEXT BUFFER DOWN BY other$buffer$size
           TO MAKE ROOM FOR THE OTHER TEXT BUFFER */

        first_o_command = _FALSE;

        Check_window(other_buffer_size);
        Movmem(oa.high_s, oa.high_s - other_buffer_size,
            (word)(oa.high_e - oa.high_s + 1));
        oa.high_s = oa.high_s - other_buffer_size;
        oa.high_e = oa.high_e - other_buffer_size;
        start_at = oa.high_e + 4;
        cursor = cursor - other_buffer_size;
        oa.bol = oa.bol - other_buffer_size;

        /*    SAVE POINTERS TO MAIN TEXT    */
        Flip_pointers();

        /*    SET UP POINTERS FOR THE OTHER BUFFER    */
        oa.low_s = start_at;                    /* WASTE A FEW BYTES    */
        low_s_byte = LF;                    /* LOW STOPPER LF */
        oa.low_s = oa.low_s + 1;
        oa.high_e = oa.low_s + other_buffer_size - 8;/* WASTE A FEW MORE BYTES */
        Read_input_file();                /* READ IN THE OTHER FILE */
        if (excep == 0)
            Display_filenames();
        V_cmnd();
    }
    else {
        Flip_pointers();       /* EXCHANGE THE BUFFER SPECIFIC INFO. */
        Display_filenames();
        V_cmnd();
    }
} /* o_cmnd */



/**************************************************************************/
/*                            q_cmnd                                      */
/*                                                                        */
/* THE QUIT COMMAND                                                       */
/*                                                                        */
/**************************************************************************/
void Q_cmnd() {
    byte ch, i, nfile, q_row, q_col;
    word dummy;
    byte rflag;
    byte eflag;
    connection conn;
    word texcep;
    char fname[_MAX_PATH];
    byte rename_done;   /* dhj  4/24/89 */

    rename_done = false;        /* dhj  4/24/89 */

/*    REMIND THE USER WHAT HE IS EDITING    */

    quit_state = 1;

    Display_filenames();

    Set_tag(ed_tagi, oa.high_s);  /* IF SPILLING, MAY HAVE TO MOVE */

    q_row = row;
    q_col = col;

    /*    PROMPT FOR SUB COMMAND. NEED DO FOREVER BECAUSE WRITE AND UPDATE
        STAY IN QUIT MODE    */

    while (1) {

        /*    LAST_CMD TELLS INPUT COMMAND THAT THE QUIT COMMAND IS IN PROGRESS
            AND THE MESSAGE LINE SHOULD BE LEFT INTACT    */
        command = 'Q';

        /*    ONLY ALLOW A 'RESAVE' IF THERE WAS AN INPUT FILENAME AND NO TEXT
            WAS LOST. IF MODE IS FOREWARD_ONLY AND THERE IS MORE INPUT,
            ASSUME THAT TEXT IS LOST.    */

        if (oa.file_disposition == view_only) {
            ch = Input_command("\x4f"
                RVID "Abort               Init               "
                RVID "                                      ");
            if (ch == 'U' || ch == 'E' || ch == 'W')
                ch = 0; /* Illegal command */
        }
        else if ((oa.input_name[0] != 0) && (oa.file_disposition != lost_file)) {
            ch = Input_command("\x4f"
                RVID "Abort     Exit      Init      Update   "
                RVID "Write                                 ");
        }
        else {
            ch = Input_command("\x4f"
                RVID "Abort               Init               "
                RVID "Write                                 ");
            if (ch == 'U' || ch == 'E')
                ch = 'W';
        }


        if (ch == 'W') {        /* IF W, NEED OUTPUT FILENAME */
            quit_state = 2;
        redo:
            if (Input_filename("\xd" "Output file: ", s_write_file) == CONTROLC) {
                Q_cleanup();
                quit_state = 0;
                return;
            }
            if (input_buffer[0] == 0) {
                Error(invalid_null_name);
                Jump_tagi();
                quit_state = 0;
                return;
            }

            if (Test_file_existence(util_file))
                goto redo;
            Put_goto(q_col, q_row);
            quit_state = 1;
        }

        /*    THE Write, Update and Exit OPTIONS ALL CAUSE THE FILE TO BE WRITTEN */

        if (ch == 'E' || ch == 'U' || ch == 'W') {
            if (ch == 'E')
                quit_state = 3;
            while (oa.more_input) {        /* MUST READ ALL OF INPUT FILE */
                i = Forward_file();
            }
#ifndef MSDOS
            /*
             * Begin R2 Fix, Part 1, PPR 2988 AF to a remote dir and then during AEDIT
             *               causes a GP error, 1/23/87
             */
             /* fix removed... unnecessary   dhj  4/24/89 */
             /*            IF ch <>'W' and ch <>'E' THEN  */
             /*
              * End R2 Fix, Part 1, PPR 2988
              */
            Get_access_rights(oa.input_name);
#endif
            Init_str(tmp_str, sizeof(tmp_str));
            if (ch == 'W') {        /* OUTPUT IS SPECIFIED */
                nfile = util_file;
                Add_str_str(input_buffer);
                Print_name(tmp_str);
            }
            else if (oa.output_name[0] == 0) {    /* OUTPUT IS INPUT */
                nfile = in_file;
                Add_str_str(oa.input_name);
                Print_name(tmp_str);
                if (oa.new_file == _FALSE) {
                    if (backup_files) {
                        /* THE BACKUP_FILES FLAG SAYS WHETHER
                           OR NOT A .BAK FILE IS NEEDED    */
                           /*
                            * Begin R2 Fix, Part 2, PPR: 2984, a problem with multi-access qe/qw...,
                            *               2/3/87
                            */
                        rflag = _FALSE;
                        /* close the input file connection opened at start */
                        if (files[in_file].conn != NULL) {
                            Detach(in_file);
                            if ((excep != 0)) {
                                quit_state = 0;
                                return;
                            }
                            oa.more_input = _FALSE;
                            rflag = _TRUE;
                        }
                        toCstr(fname, oa.input_name);
                        if (_access(fname, 0) == 0) {                   // exists
                            if ((conn = fopen(fname, "a")) == NULL) {   // but can't write to it
                                if (errno == EACCES) {
                                    texcep = excep = errno;
                                    Echeck();
                                    if (rflag)
                                        Openi(in_file, 2);
                                    quit_state = 0;
                                    excep = texcep;
                                    return;
                                }
                            }
                            else {
                                excep = fclose(conn) ? errno : 0;
                                Echeck();
                            }
                        }
                        /*
                         * End R2 Fix, Part 2, PPR: 2984
                         */
                         /* only after renaming do we need to
                          * modify the access rights. */
                        rename_done = true; /* dhj  4/24/89 */
                        Rename_as_bak();
                        if (excep != 0) {
                            texcep = excep;
                            Jump_tagi();
                            if (rflag)
                                Openi(in_file, 2);
                            quit_state = 0;
                            excep = texcep;
                            return;
                        }
                    }
                }
            }
            else {                        /* OUTPUT IS 'TO' FILE */
                nfile = out_file;
                Add_str_str(oa.output_name);
                Print_name(tmp_str);
            }
            /*
             * Begin R2 Fix, Part 3, PPR: 2984, a problem with multi-access qe/qw...,
             *               2/3/87
             */
             /* close the input file connection opened at start */
            rflag = _FALSE;
            eflag = _FALSE;
            if (files[nfile].conn != NULL) {
                Detach(nfile);
                if ((excep != 0)) {
                    quit_state = 0;
                    return;
                }
                oa.more_input = _FALSE;
                eflag = _TRUE;
            }
            if (Cmp_name_insensitively(files[nfile].name, files[in_file].name) && !eflag) {
                if (files[in_file].conn != NULL) {
                    Detach(in_file);
                    if ((excep != 0)) {
                        quit_state = 0;
                        return;
                    }
                    oa.more_input = _FALSE;
                    rflag = _TRUE;
                }
            }
            toCstr(fname, files[nfile].name);
            // check we can create the output file
            if (_access(fname, 0) == 0) {           // file exists
                if ((conn = fopen(fname, "a")) == 0) {  // can't open it
                    if (errno == EACCES) {
                        texcep = excep = errno;
                        Echeck();
                        if (eflag)
                            Openi(nfile, 2);
                        if (rflag)
                            Openi(in_file, 2);
                        quit_state = 0;
                        excep = texcep;
                        return;
                    }
                    else {
                        excep = fclose(conn) ? errno : 0;
                        Echeck();
                    }
                }
            }

            /*
             * End R2 Fix, Part 3, PPR: 2984
             */
            Write_entire_file(nfile); /* WRITE ENTIRE TEXT CONTENTS */
            if (excep != 0) {
                texcep = excep;
                Jump_tagi();
                /* open the input file connection again */
                if (eflag)
                    Openi(nfile, 2);
                if (rflag)
                    Openi(in_file, 2);
                quit_state = 0;
                excep = texcep;
                return;
            }
            if (eflag)
                Openi(nfile, 2);
            if (rflag)
                Openi(in_file, 2);
#ifndef MSDOS
            /*
             * Begin R2 Fix, Part 2, PPR 2988 AF to a remote dir and then during AEDIT
             *               causes a GP error, 1/23/87
             */
             /* fix modified to set access rights only if renaming  dhj 4/24/89 */
             /*            IF ch <>'W' and ch <>'E' THEN */
            if (rename_done)
                /*
                 * End R2 Fix, Part 2, PPR 2988
                 */
                Put_access_rights(files[nfile].name);
#endif
            Add_str_str("\x11" " has been written");
            if (tmp_str[0] > string_len)
                tmp_str[0] = string_len;
            Move_name(tmp_str, input_buffer);  /* PUT END MSG ASIDE */
            oa.dirty = _FALSE;
            if (ch != 'W')
                oa.new_file = _FALSE;

            /* OTHERWISE SEE IF OTHER BUFFER MIGHT NEED RESAVING */
            if (ch == 'E') {/* USER IS ALL DONE */
                                        /* SEE IF STILL NEED OTHER */
                if (Keep_other_after_all()) {
                    quit_state = 0;
                    return;
                }
                oa.file_disposition = keep_file; /* Dont print 'View,Forward' */
                in_other = _FALSE;            /* DONT PRINT 'Other ' */
                Print_message(input_buffer);
                Quit_exit(0);
            }
            Print_message(input_buffer);

        }
        else if (ch == 'A' || ch == 'I') {
            quit_state = 0;
            if (ch == 'A')
                quit_state = 3;
            if (oa.dirty && oa.file_disposition != lost_file) {
                Display_filenames();
                if (Keep_after_all()) {    /* USER CHANGED MIND    */
                    Q_cleanup();
                    quit_state = 0;
                    return;
                }
            }
            oa.dirty = _FALSE;                    /* USER DOES NOT WANT    */
            if (ch == 'A') {
                if (Keep_other_after_all()) {
                    quit_state = 0;
                    return;
                }
                Quit_exit(0);
            }

            if (s_init[0] == 0) {
                /* If s_init is empty (before first QI),
                   get names from invocation line. */
                Init_str(s_init, string_len_plus_1);
                Add_str_str(s_input_file);
                if (s_output_file[0] != 0) {
                    Add_str_str("\x4" " TO ");
                    Add_str_str(s_output_file);
                }
                if (oa.file_disposition == view_only) {
                    Add_str_str("\x3" " VO");
                }
                else if (oa.file_disposition == lose_file ||
                    oa.file_disposition == lost_file) {
                    Add_str_str("\x3" " FO");
                }
            }
            if (Input_filename("\x22" "enter [file [TO file | VO | FO]]: ", s_init) == CONTROLC) {
                Q_cleanup();
            }
            else {
                input_buffer[input_buffer[0] + 1] = CR;   /* FORCE END OF LINE */
                dummy = dq_switch_buffer(&input_buffer[1], &excep);
                in_invocation_line = _FALSE;
                delim = ' ';
                at_eoln = _FALSE;
                tmp_str[0] = 0;
                was_error = _FALSE;
                Get_arg();
                Get_input_file_name();
                if (arg_type != ctl_eoln && !was_error) {
                    Invocation_error();
                }
                if (!was_error) {
                    Read_input_file();
                    if (excep == 0)
                        Kill_message();
                }
                V_cmnd();
            }
            quit_state = 0;
            return;
        }
        else if (ch == CONTROLC || ch == esc_code) {
            Q_cleanup();
            quit_state = 0;
            return;
        }
        else {
            Illegal_command();
        }
    }
} /* q_cmnd */
