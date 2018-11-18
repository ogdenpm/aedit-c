// #SMALL
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

/* general publics */
#include <stdio.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"


/*                COMMENT

|*    SOME VARIABLES NEED TWO VERSIONS - ONE FOR EACH TEXT BUFFER.    
      THE VARIABLES CURRENTLY IN USE ARE IN STRUCTURE OA. THE STRUCTURE OB
      CONTAINS THE VARIABLES FOR THE NOT CURRENT VARIABLES. *|


NOTE
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ! when updating oa, update the structure TWO_OF !
    ! in the assembly language module.              !
    ! (including array sizes and initial values.)   !
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    oa_struc
    struct {

    |* THE FOLLOWING VARIABLES KEEP TRACK OF TEXT SPILLED TO OVERFLOW FILES
       SEE IO FOR DETAILS *|

    |* TRUE IF STILL MORE TEXT IN THE ORIGINAL INPUT FILE *|
        more_input BYTE,

    |* THE FILE_DISPOSITION FLAG SAYS AND SHOWS IF A FILE MUST BE SPILLED
       TO DISK IF IT OVERFLOWS. THE 'OTHER' FILE AND ANY FILE INITIALIZED
       WITH A COMMAND TAIL CONTAINING 'TO :BB:' IS FLAGGED LOSE_FILE.
       IF BLOCKS ARE ACTUALLY DISCARDED, FILE_DISPOSITION IS CHANGED TO LOST_FILE
       TO INFORM THE QUIT ROUTINE THAT SOMETHING IS LOST. *|
        file_disposition BYTE,

    |* NUMBER OF BLOCKS OF DATA IN THE FIRST WORK FILE. A BLOCK
       IS BLOCK_SIZE BYTES. THE FIRST BLOCK IS THE ZERO BLOCK *|
        wk1_blocks WORD,

    |* NUMBER OF BLOCKS IN WK2. *|
        wk2_blocks WORD,

    |* THE PORTION OF THE USERS TEXT IS ORGANIZED IN MEMORY AS FOLLOWS
       <-LF->< -LOW BLOCK -><- GAP -><- HIGH BLOCK -><- "|" -><-LF->
       LOW_S    POINTS TO FIRST BYTE OF LOW BLOCK
       LOW_E    POINTS TO THE FIRST BYTE AFTER LOW BLOCK WHICH IS ALWAYS A LF 
       HIGH_S   POINTS TO THE FIRST BYTE OF HIGH BLOCK
       HIGH_E   POINTS TO THE LF THAT FOLLOWS HIGH BLOCK
       CURSOR   NORMALLY HAS THE SAME VALUE AS HIGH_S (WHICH REFLECTS THE
                LOGICAL POSITION OF THE SCREEN CURSOR) BUT MOVES ABOUT
                DURING COMMANDS.*|
        low_s WORD,
        low_e WORD,
        high_s WORD,
        high_e WORD,


    |* THE COMMAND TAIL VARIABLES *|
        input_name(46) BYTE,
        output_name(46) BYTE,

        new_file BYTE,
        have_eof BYTE,
    |* THE FOUR USER AND FOUR EDITER TAGS ARE DEFINED IN THE FOLLOWING.
       THE USER TAGS ARE THE FIRST 4- 'A' 'B' 'C' AND 'D'. TAG 0 IS NOT USED.
       IF BLOCK = -1 THEN THE TAG IS UNDEFINED.
       IF BLOCK < WK1_BLOCKS THEN BLOCK IS IN A BLOCK IN WK1 AND OFFSET IS
       THE OFFSET (STARTING WITH ZERO) OF THAT BLOCK.
       IF BLOCK = WK1_BLOCKS THEN THE TAG REFERS TO A LOCATION IN MEMORY AND
       OFFSET IS AN ACTUAL MEMORY LOCATION.
       IF BLOCK > WK1_BLOCKS THEN THE TAG REFERS TO A LOCATION IN WK2.
       A TAG CAN NEVER REFER TO A LOCATION IN THE SOURCE FILE. *|
        tblock(9) WORD,
        toff(9) WORD,

    |* THE CONNECTION NUMBER TO THE INPUT FILE. NOTE THAT THIS IS NOT UPDATED
       SO ONLY THE VALUE IN STRUCTURE OB IS CORRECT.    *|
        in_conn WORD,

    |* TRUE IF FILE HAS BEEN CHANGED *|
        dirty BYTE,

    |* FIRST COLUMN OF A LINE THAT SHOULD BE DISPLAYED *|
        left_column BYTE,

    |* Beginning of the line that the cursor is currently on - used by
       backup_line to determine the beginning of a line without a linefeed.
       This provides synchronization for reviewing, so things that are
       at the first of the visual lines will remain there. *|
        bol WORD,

        wk1_conn BYTE,
        wk2_conn BYTE
    );

END OF COMMENT */



/*
                   NOTE
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ! when updating oa, update the structure TWO_OF !
    ! in the assembly language module.              !
    ! (including array sizes.)                      !
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/


    oa_struc oa = {
        0, _FALSE, keep_file, 0, 0, NULL, NULL, NULL, NULL, {0}, {0}, 0, 0,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        {0}, NULL, _FALSE, 0, 0, 0, 1};

/*    THE OB STRUCTURE IS NOT USED. IT IS ONLY A PLACE TO STORE VALUES
    THAT RELATE TO THE NOT CURRENT FILE    */

    oa_struc ob = {
        0, _FALSE, keep_file, 0, 0, NULL, NULL, NULL, NULL, {0}, {0}, 0, 0,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        {NULL}, NULL, _FALSE, 0, 0, 2, 3};


    /* this array tells whether or not the given character is a delimiter
       or not 1->delimiter, 0-> non delimiter */

    byte delimiters[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* all control characters */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* $ and _ are non_delimiters */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* all chars above 7FH are dels. */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        


    /* window_file=0 if upper window is on main file, 0FFH if on other file */
    /*p    DCL window_file BYTE PUBLIC INITIAL(0);*/

    /*    when any edit is done on the file that the upper window is pointing
        to, this byte will be set, so when we go to display the upper window,
        we can replot it
    */
    byte w_dirty;



    pointer cursor;


/*    SCREEN VARIABLES */

    byte row, col;            /* CURRENT LOCATION OF PHYSICAL CURSOR */
    byte virtual_col;        /* COLUMN NUMBER THAT DOES NOT REFLECT
                                            LEFT_COLUMN SUBTRACTION */
    byte wrapping_column = {255};    /* line size before
                                                            virtual-LF insert
                                                        */
    pointer have[65] = {0};    /* ADDRESSES OF TEXT ON THE SCREEN */
    pointer saved_from = {0};    /* LOCATION WHERE SAVED LINE STARTS */
    byte saved_bytes[81];    /*  IMAGE OF ABOVE LINE */
    byte line_size[66] = {    /* CURRENT LENGTH OF EACH LINE */
        80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80};



    byte first_text_line = 0, last_text_line = 0, message_line = 0, prompt_line = 0;

    /* The following structure is used to store the values for the
       alternate window on the screen (if present) */
    window_t window;

    bool window_present = false;    /* Set if screen split */
    byte window_num = {1};        /* If a window is present,  then this
                                           byte contains 0 for the upper window,
                                           and 1 for the lower one
                                        */



    byte force_column = {100};/* USED BY UP AND DOWN TO FORCE
                                            THE DISPLAY COLUMN */
    byte command;            /* CURRENT COMMAND */
    byte last_cmd = {0};    /* USED BY HOME COMMAND */
    byte last_main_cmd = {0};    /* USED BY 
                                                        SUPPLY_MACRO_CHAR */
    word count;                /* REPETITION COUNT */
    byte infinite;            /* whether or not to do for ever */


/*    THE FOLLOWING ARE THE ENVIRONMENTAL OPTIONS THAT NEED TO BE KNOWN
    GLOBALLY
*/
    byte auto_indent = {_FALSE};
    byte blank_for_tab = {_FALSE};
    byte crlf_insert = {_FALSE};
    byte find_uplow = {_TRUE};
    byte show_find = {_FALSE};
    byte token_find = {_FALSE};
    byte backup_files = {_TRUE};
    byte center_line = {5};
    byte dont_stop_macro_output = {_FALSE};
    byte radix = {10};

    boolean highbit_flag = {_FALSE};
       /* highbit handling: printable (CONSOL.PLM)
          unfold_to_screen (AEDASM.ASM) */

    byte lstfnd = {0};
    byte cursor_type;
    

/*
    TAB_TO        CONTAINS THE NUMBER OF SPACES THAT SHOULD BE OUTPUT
                WHEN A TAB IS FOUND IN THE CORRESPONDING COLUMN 
                DEFAULT IS TAB EVERY 4 COLUMNS
*/
    byte tab_to[255] = {
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
        4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2};



    byte cc_flag = {_FALSE};

    byte have_control_c = {0};
      /* TURNED ON BY CONTROL C ROUTINE */

    byte quit_state = {0xff};
       /* used to determine type-ahead shut off */

    byte input_buffer[string_len_plus_1 + 1];  /* INPUT LINES ARE PUT HERE */
      /* Some procedures put a CR after the input_buffer.
         I assume allocation is contigious. I am not using AT
         because AT doesn't allocate memory. */

    byte tmp_str[81];
      /* MESSAGES ARE ACCUMULATED HERE */

    byte in_macro_def = {_FALSE};
      /* TRUE IF IN INTERRACTIVE MACRO DEFINITION MODE */

    byte in_other = {_FALSE};
      /* TRUE IF IN OTHER FILE */

    byte w_in_other = {_FALSE};
      /* TRUE IF THE WINDOW IS IN OTHER FILE */

    byte macro_exec_level = {0};
      /*    CURRENT DEPTH OF MACROS    */

    byte macro_suppress = {_FALSE};
      /* TRUE IF OUTPUT SUPPRESSED FOR MACRO DURATION */

    byte minus_type;
      /* TRUE FOR -FIND */

    word excep;
      /* EXCEPTION WORD FOR DQ CALLS */

    byte hex_digits[] = {"0123456789ABCDEF"};

    logical isa_text_line = {_FALSE};
    logical in_block_mode = {_FALSE};
    logical batch_mode = {_FALSE};
    logical submit_mode = {_FALSE};

    /* WORD processing variables */

    byte left_margin = {0};
    byte right_margin = {76};
    byte indent_margin = {4};

    byte debug = {_FALSE};
    


    byte ci_name[] = {"\x4" ":CI:"};
    byte co_name[] = {"\x4" ":CO:"};
    byte work_name[] = {"\x6" ":WORK:"};

/* THE FOLLOWING STRUCTURE MAINTAINS THE FILE INFORMATION */
    file_t files[7] = {
        ci_name,         NULL,
        co_name,         NULL,
        s_macro_file,    NULL,
        oa.input_name,   NULL,
        oa.output_name,  NULL,
        input_buffer,    NULL,
        work_name,       NULL };



    byte file_num;        /* NUMBER OF CURRENT FILE.      */



   byte co_buffer[255] = {0};

   byte null_str = {0}; /* a null string */

   boolean force_writing = {_FALSE};

   boolean do_type_ahead = {1}; /* undefined */

   word delay_after_each_char = {0};
     /* delay after each char. */

   byte invalid_null_name[] = {"\x11" "invalid null name"};


