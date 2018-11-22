#pragma once
#include <stdbool.h>
#include <stdio.h>
typedef unsigned char byte;
typedef unsigned short word;
typedef word wbyte;    // wbyte is something that can be a byte, but is a word on 86 for better code generation
typedef unsigned int dword; // make sure sdword has same underlying size
typedef int sdword;
typedef FILE *connection;
typedef byte *pointer;
typedef word *wpointer;
typedef dword *dpointer;
typedef byte logical;
typedef byte boolean;
#pragma pack(push, 1)
typedef union {
    pointer bp;
    wpointer wp;
    word w;
    dword dw;
} address_t;

typedef union {
    pointer bp;
    word offset;
} toff_t;

/*              NOTE
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ! when updating oa, update the structure TWO_OF !
    ! in the assembly language module.              !
    ! (including array sizes and initial values.)   !
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

/* THE FOLLOWING VARIABLES KEEP TRACK OF TEXT SPILLED TO OVERFLOW FILES
SEE IO FOR DETAILS */

typedef struct _oa {
    word  block_size;
    byte  more_input;           /* TRUE IF STILL MORE TEXT IN THE ORIGINAL INPUT FILE */
    byte  file_disposition;     /* THE FILE_DISPOSITION FLAG SAYS AND SHOWS IF A FILE MUST BE SPILLED
                                   TO DISK IF IT OVERFLOWS.THE 'OTHER' FILE AND ANY FILE INITIALIZED
                                   WITH A COMMAND TAIL CONTAINING 'TO :BB:' IS FLAGGED LOSE_FILE.
                                   IF BLOCKS ARE ACTUALLY DISCARDED, FILE_DISPOSITION IS CHANGED TO LOST_FILE
                                   TO INFORM THE QUIT ROUTINE THAT SOMETHING IS LOST. */
    
    word  wk1_blocks;           /* NUMBER OF BLOCKS OF DATA IN THE FIRST WORK FILE.A BLOCK
                                   IS BLOCK_SIZE BYTES.THE FIRST BLOCK IS THE ZERO BLOCK */
    word  wk2_blocks;           /* NUMBER OF BLOCKS IN WK2. */
    /* THE PORTION OF THE USERS TEXT IS ORGANIZED IN MEMORY AS FOLLOWS
        < -LF-> < -LOW BLOCK -> < -GAP -> < -HIGH BLOCK -> < -"|" -> < -LF->
        LOW_S    POINTS TO FIRST BYTE OF LOW BLOCK
        LOW_E    POINTS TO THE FIRST BYTE AFTER LOW BLOCK WHICH IS ALWAYS A LF
        HIGH_S   POINTS TO THE FIRST BYTE OF HIGH BLOCK
        HIGH_E   POINTS TO THE LF THAT FOLLOWS HIGH BLOCK
        CURSOR   NORMALLY HAS THE SAME VALUE AS HIGH_S(WHICH REFLECTS THE
            LOGICAL POSITION OF THE SCREEN CURSOR) BUT MOVES ABOUT
        DURING COMMANDS
     */
    pointer low_s;              
    pointer low_e;
    pointer high_s;
    pointer high_e;
    byte  input_name[string_len_plus_1];        /* THE COMMAND TAIL VARIABLES note dimension is 61 */
    byte  output_name[string_len_plus_1];
    byte  new_file;
    byte  have_eof;
    /* THE FOUR USER AND FOUR EDITER TAGS ARE DEFINED IN THE FOLLOWING.
       THE USER TAGS ARE THE FIRST 4 - 'A' 'B' 'C' AND 'D'.TAG 0 IS NOT USED.
       IF BLOCK = -1 THEN THE TAG IS UNDEFINED.
       IF BLOCK < WK1_BLOCKS THEN BLOCK IS IN A BLOCK IN WK1 AND OFFSET IS
       THE OFFSET(STARTING WITH ZERO) OF THAT BLOCK.
       IF BLOCK = WK1_BLOCKS THEN THE TAG REFERS TO A LOCATION IN MEMORY AND
       OFFSET IS AN ACTUAL MEMORY LOCATION.
       IF BLOCK > WK1_BLOCKS THEN THE TAG REFERS TO A LOCATION IN WK2.
       A TAG CAN NEVER REFER TO A LOCATION IN THE SOURCE FILE
     */
    word  tblock[9];
    toff_t toff[9];
    connection  in_conn;          /* THE CONNECTION NUMBER TO THE INPUT FILE.NOTE THAT THIS IS NOT UPDATED
                               SO ONLY THE VALUE IN STRUCTURE OB IS CORRECT.*/
    byte  dirty;            /* TRUE IF FILE HAS BEEN CHANGED */
    byte  left_column;      /* FIRST COLUMN OF A LINE THAT SHOULD BE DISPLAYED */
    pointer  bol;           /* Beginning of the line that the cursor is currently on - used by
                               backup_line to determine the beginning of a line without a linefeed.
                               This provides synchronization for reviewing, so things that are
                               at the first of the visual lines will remain there*/
    byte  wk1_conn;
    byte  wk2_conn;
} oa_struc;


typedef union {
    struct {
        byte cur_byte, next_byte;
    } curs;
    struct {
        word cur_word, next_word;
    } cursw;
    word cursor_word;
    byte cursor_byte;
    byte cursor_bytes[1];
} cursor_t;


typedef struct { byte _char;  void(*cmnd)(); } dispatch_t;
typedef struct { pointer name;  FILE *conn; } file_t;


/* The following structure is used to store the values for the
   alternate window on the screen (if present) */
typedef struct {
    byte first_text_line;
    byte last_text_line;
    byte message_line;
    byte prompt_line;
    byte prompt_number;
    byte current_message[81];    /* CONTENTS OF WINDOW MESSAGE LINE */
    byte last_message[81];
    byte current_prompt[81];     /*CONTENTS OF WINDOW PROMPT LINE*/
    byte suppressed_prompt[81];
} window_t;


typedef struct {
    dword system_time;
    byte date[8], time[8];
} dt_t;

typedef struct {
    byte date[8], time[8];
} old_dt_t;

typedef struct {
    byte owner[15];
    dword length_offset;
    byte type, owner_access, world_access /*, group_access */;
    dword create_time, last_mod_time;
    byte reserved[20];
} file_info_t;

typedef struct {
    word long_offset;
    word long_base;
} long_p_t;

typedef struct {
    boolean open;
    byte access, seek;
    dword  file_ptr;
} info_t;

typedef struct {
    byte code[5];
} code_t;

typedef union {
    struct {
        byte length;
        dword dw;
    };
    byte code[5];
} match_t;

typedef struct {
    byte len;
    pointer p;
} entry_t;

typedef struct {
    word text_length;
    byte text[1];
} macrotext_t;


typedef struct {
    boolean in_invocation,
        /* during invocation there is a special error handling,
           because there may be more than one error, and we want
           to see all of them, and because the configuration is
           not yet set. */
        from_macro_file,
        /* there may be many errors during macro-file processing,
           so we seperate them by hit_space. */
        key_was_space;
        /* inform macro_file_error if a space was hit in order
           to continue, or another char in order to stop reading. */
} error_struct_t;

typedef void(*exception_handler_t)(word excep_code, word param_num, word res, word status);

typedef unsigned short selector;    // to eliminate as base of a long pointer

typedef struct {
    word offset;
    selector token;
} selector_t;

typedef struct {
    boolean open;
    byte access;
    byte seek;
    word filepl;
    word fileph;
} conn_info_t;
#pragma pack(pop)
