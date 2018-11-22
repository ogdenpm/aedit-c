// #SMALL
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/
#include <stdbool.h>
#include <time.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"

static pointer Get_string_variable(byte ch);
static dword N_stat_in_parens();

static byte ff = 0xff;      // used to allow address to be taken
// size & sign bit info for dwords
const int nbits = sizeof(dword) * 8;
const dword signBit = 1 << (sizeof(dword) * 8 - 1);

byte invalid_name[] = { "\x15" "invalid variable name" };



/*****************************************************************
    Global string Variables.
*****************************************************************/

byte s0[string_len_plus_1] = { 0 };
byte s1[string_len_plus_1] = { 0 };
byte s2[string_len_plus_1] = { 0 };
byte s3[string_len_plus_1] = { 0 };
byte s4[string_len_plus_1] = { 0 };
byte s5[string_len_plus_1] = { 0 };
byte s6[string_len_plus_1] = { 0 };
byte s7[string_len_plus_1] = { 0 };
byte s8[string_len_plus_1] = { 0 };
byte s9[string_len_plus_1] = { 0 };

byte s_write_file[filename_len_plus_1] = { 0 };
byte s_get_file[filename_len_plus_1] = { 0 };
byte s_put_file[filename_len_plus_1] = { 0 };
byte s_macro_file[filename_len_plus_1] = { 0 };
byte s_macro_name[filename_len_plus_1] = { 0 };


/* Pointers to global string variables. */
pointer s_var[] = {
         s0, s1, s2, s3, s4, s5, s6, s7, s8, s9,
         s_write_file,
         s_get_file,
         s_put_file,
         s_macro_file,
         target,
         replacement,
         s_input_file };


byte s_var_names[] = { "\x14" "0123456789WGPMTREIOB" };



byte param[string_len_plus_1];


boolean string_error;
/* prevents c_cmnd from deleting an error message which
   have been printed by get_string_variable */


pointer Get_s_var() {

    byte ch;

    Print_message("\x6" "<FETS>");
    ch = Upper(Cmd_ci());
    Clear_message();
    if (ch == CONTROLC)
        return &ff;
    return Get_string_variable(ch);
} /* get_s_var */





/* Reads the second char of a s_var name, and returns a pointer to that string. */
static pointer Get_string_variable(byte ch) {
    byte index;


    string_error = _FALSE;
    if (ch == 'B') {
        if (in_block_buffer > string_len) {
            /* block_buffer contains too many chars,
               or the block is written to file */
            Error("\x1d" "block buffer too large for SB");
            string_error = _TRUE;
            return &null_str;
        }
        param[0] = (byte)in_block_buffer;
        memcpy(&param[1], &block_buffer[0], param[0]);
        return param;
    }
    if (ch == 'I')
        if (in_other)
            return ob.input_name;
        else
            return oa.input_name;

    if (ch == 'O')
        if (in_other)
            return oa.input_name;
        else
            return ob.input_name;

    index = Find_index(ch, s_var_names);
    if (index != s_var_names[0]) {
        /* The name is legal. */
        return s_var[index];
    }
    else {
        Error(invalid_name);
        string_error = _TRUE;
        return &null_str;
    }

} /* get_string_variable */






/*****************************************************************
    Global numeric Variables.
*****************************************************************/

#define NMAX	9

/* Global numeric variables. */
dword n_var[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };




byte n_str[8];



pointer Get_n_var(byte radix, boolean add_minus) {

    byte ch;
    byte len;
    byte i;
    pointer p;
    dword dtemp;

    Print_message("\x6" "<FETN>");
    ch = Cmd_ci();
    Clear_message();
    if (ch >= '0' && ch <= '9') {
        if (ch == CONTROLC)
            return &ff;
        dtemp = n_var[ch - '0'];

        if (radix != 0) {
            if (radix == 10 && (dtemp & signBit)) {
                dtemp = ~dtemp + 1;     // -dtemp
                p = Print_number(dtemp, 0, 10);
                if (add_minus) {
                    Move_name(p, &n_str[1]);
                    n_str[0] = n_str[1] + 1;
                    n_str[1] = '-';
                }
                else {
                    Move_name(p, &n_str[0]);
                }
                return n_str;
            }
            return Print_number(dtemp, 0, radix);
        }
        else {    /* radix is ascii */
            len = 4;
            if (dtemp < 256) {
                len = 1;
            }
            else if (dtemp < 65536) {
                len = 2;
            }
            for (i = 1; i <= len; i++) {
                n_str[i] = (byte)dtemp;
                dtemp >>= 8;
            }
            n_str[0] = len;
            return n_str;
        }
    }
    else {
        Error(invalid_name);
        return &null_str;
    }

} /* get_n_var */





pointer tok_ptr;
byte token;
byte token_subtype;
dword token_val;
pointer temp_tok_ptr;
byte temp_tok;


byte s_calc[string_len_plus_1] = { 0 };
/* Holds last input-line read by CALC for reediting. */

byte string[81]; /* was 256   IB */

boolean force;
/* if we are in macro execution, the message is printed only
   if the top operation is not an assignment.
   values: 0 - we don't know yet. 1 - don't print.
           2 - print. 3 - ignore temporarily. */





           /* Char types */

#define ch_error	0x0
#define ch_number	0x1
#define ch_equal	0x2
#define ch_less	0x3
#define ch_great	0x4
#define ch_plsmin	0x5
#define ch_lpar	0x6
#define ch_rpar	0x7
#define ch_cr	0x8
#define ch_unary	0x9
#define ch_star	0xa
#define ch_divmod	0xb
#define ch_logic	0xc
#define ch_letter	0xe


/* The lower nibble is the type, and the upper is the subtype (if needed). */

 /* Char subtypes */

#define ch_mul	0x0
#define ch_div	0x10
#define ch_mod	0x20
#define ch_or	0x0
#define ch_and	0x10
#define ch_xor	0x20
#define ch_pls	0x0
#define ch_min	0x10
#define ch_exc	0x20
#define ch_tld	0x30
#define ch_sgn	0x40



byte char_type_tbl[] = {

    /*00*/ ch_error        , ch_error        , ch_error        , ch_error        ,
    /*04*/ ch_error        , ch_error        , ch_error        , ch_error        ,
    /*08*/ ch_error        , ch_error        , ch_error        , ch_error        ,
    /*0C*/ ch_error        , ch_cr           , ch_error        , ch_error        ,
    /*10*/ ch_error        , ch_error        , ch_error        , ch_error        ,
    /*14*/ ch_error        , ch_error        , ch_error        , ch_error        ,
    /*18*/ ch_error        , ch_error        , ch_error        , ch_error        ,
    /*1C*/ ch_error        , ch_error        , ch_error        , ch_error        ,
    /*20*/ ch_error        , ch_unary + ch_exc , ch_error        , ch_unary + ch_sgn ,
    /*24*/ ch_error        , ch_error        , ch_logic + ch_and , ch_error        ,
    /*28*/ ch_lpar         , ch_rpar         , ch_star + ch_mul  , ch_plsmin + ch_pls,
    /*2C*/ ch_error        , ch_plsmin + ch_min, ch_error        , ch_divmod + ch_div,
    /*30*/ ch_number       , ch_number       , ch_number       , ch_number       ,
    /*34*/ ch_number       , ch_number       , ch_number       , ch_number       ,
    /*38*/ ch_number       , ch_number       , ch_error        , ch_error        ,
    /*3C*/ ch_less         , ch_equal        , ch_great        , ch_error        ,
    /*40*/ ch_error        , ch_letter       , ch_letter       , ch_letter       ,
    /*44*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*48*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*4C*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*50*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*54*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*58*/ ch_letter       , ch_letter       , ch_letter       , ch_error        ,
    /*5C*/ ch_divmod + ch_mod, ch_error        , ch_logic + ch_xor , ch_error        ,
    /*60*/ ch_unary + ch_tld , ch_letter       , ch_letter       , ch_letter       ,
    /*64*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*68*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*6C*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*70*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*74*/ ch_letter       , ch_letter       , ch_letter       , ch_letter       ,
    /*78*/ ch_letter       , ch_letter       , ch_letter       , ch_error        ,
    /*7C*/ ch_logic + ch_or  , ch_error        , ch_unary + ch_tld , ch_error        ,
    /*80*/ ch_error        , ch_error };



static byte Char_type(byte ch) {

    if (ch >= sizeof(char_type_tbl))        // PMO fixed boundary index
        return 0; /* error */
    return char_type_tbl[ch] & 0xf;
}

static byte Char_subtype(byte ch) {
    if (ch >= sizeof(char_type_tbl))        // PMO fixed boundary index
        return 0; /* error */
    return (byte)(char_type_tbl[ch] >> 4);
}

struct { byte len, name[6]; } variables[50] = {
        3, "COL   ",
        3, "ROW   ",
        6, "LSTFND",
        6, "INOTHR",
        5, "ISDEL ",
        6, "ISWHTE",
        5, "CURCH ",
        4, "UPCH  ",
        5, "LOWCH ",
        5, "CURWD ",
        5, "NXTCH ",
        5, "NXTWD ",
        6, "CURPOS",
        4, "TAGA  ",
        4, "TAGB  ",
        4, "TAGC  ",
        4, "TAGD  ",
        3, "EOF   ",
        3, "BOF   ",
        4, "DATE  ",
        4, "TIME  ",
        6, "IMARGN",
        6, "LMARGN",
        6, "RMARGN",
        6, "NSTLVL",
        6, "CNTFND",
        6, "CNTREP",
        6, "CNTMAC",
        6, "CNTEXE",
        6, "NXTTAB",
        3, "SL0   ",
        3, "SL1   ",
        3, "SL2   ",
        3, "SL3   ",
        3, "SL4   ",
        3, "SL5   ",
        3, "SL6   ",
        3, "SL7   ",
        3, "SL8   ",
        3, "SL9   ",
        3, "SLB   ",
        3, "SLI   ",
        3, "SLO   ",
        3, "SLW   ",
        3, "SLG   ",
        3, "SLP   ",
        3, "SLM   ",
        3, "SLT   ",
        3, "SLR   ",
        3, "SLE   "
};


word var_index;    /* pointer to which variable we want */
old_dt_t dt;


static dword Convert_time_and_date(bool date) {     // if date true then get date else get time
    struct tm *newTime;
    time_t szClock;
    // get time in seconds
    time(&szClock);
    newTime = localtime(&szClock);

    if (date)
        return ((newTime->tm_mon + 1) * 100 + newTime->tm_mday) * 100 + newTime->tm_year % 100;
    else
        return (newTime->tm_hour * 100 + newTime->tm_min) * 100 + newTime->tm_sec;
} /* convert_time_and_date */





/*******************************************************************
    Get block number of a tag and its in the block, and return
    the offset of that tag from the file beginning.
*******************************************************************/
static dword Get_offset(word block_num, toff_t offset_in_block) {

    dword offset;

    if (block_num > oa.wk1_blocks + oa.wk2_blocks + 1)  /* Tag has no value. */
        return 0;

    if (block_num > oa.wk1_blocks) {
        offset = (block_num - oa.wk1_blocks - 1) * oa.block_size +
            offset_in_block.offset + Size_of_text_in_memory() +
            oa.wk1_blocks * oa.block_size;
    }
    else if (block_num == oa.wk1_blocks) {
        if (offset_in_block.bp >= oa.high_s) {
            offset = (word)(offset_in_block.bp - oa.high_s + oa.low_e - oa.low_s);
        }
        else {
            offset = (dword)(offset_in_block.bp - oa.low_s);
        }
        offset = offset + oa.wk1_blocks * oa.block_size;
    }
    else {
        offset = block_num * oa.block_size + offset_in_block.offset;
    }
    return offset;

} /* get_offset */



/*******************************************************************
    Causes the next <num> chars in the file to be in memory.
    If impossible - returns false.
*******************************************************************/
boolean Look_ahead(byte num) {


    if (oa.high_e - oa.high_s - 1 <= num) {
        Set_tag(ed_tagb, oa.high_s);
        Forward_file();
        Jump_tag(ed_tagb);
    }
    return oa.high_e - oa.high_s - 1 > num;

} /* look_ahead */





static byte Real_pos() {

    byte pos;
    pointer ch_ptr;

    pos = 0;
    for (ch_ptr = Backup_line(0, oa.low_e, _FALSE); ch_ptr < oa.low_e; ch_ptr++)
        if (*ch_ptr == TAB)
            pos += tab_to[pos];
        else
            pos++;
    return pos;
} /* real_pos */




static boolean Is_a_system_variable() {
    // code modified as variable len and name may not be in contiguous memory
    for (var_index = 0; var_index <= last(variables); var_index++) {
        if (variables[var_index].len == tok_ptr[0] && memcmp(variables[var_index].name, tok_ptr + 1, tok_ptr[0]) == 0)
            return _TRUE;
    }
    return _FALSE;
} /* is_a_system_variable */




static dword System_variable() {
    byte c, i;

    tok_ptr += variables[var_index].len;
    switch (var_index) {
    case 0: return Real_pos();  /* COL */
    case 1: /* ROW */
        if (macro_suppress || have[first_text_line] == 0)
            return 0;
        if (oa.high_s >= have[first_text_line] && oa.high_s < have[message_line]) {
            for (i = first_text_line + 1; oa.high_s >= have[i]; i++)
                ;
            return first_text_line + i - 1;         //PMO this looks odd, I suspect it should be i - 1
        }
        return 0;
    case 2: return lstfnd ? ~0 : 0; /* LSTFND */
    case 3: return in_other ? ~0 : 0;  /* INOTHR */
    case 4: return delimiters[((cursor_t *)cursor)->curs.cur_byte] ? ~0 : 0; /* ISDEL */
    case 5:	return findb(" \t\r\n", ((cursor_t *)cursor)->curs.cur_byte, 4) != 0xffff ? ~0 : 0;  /* ISWHTE */
    case 6:	return Look_ahead(0) ? ((cursor_t *)cursor)->curs.cur_byte : 0;  /* CURCH */
    case 7:	return Look_ahead(0) ? Upper(((cursor_t *)cursor)->curs.cur_byte) : 0;   /* UPCH - upper case of current byte. */
    case 8:	/* LOWCH - lower case of current byte. */
        if (!Look_ahead(0))
            return 0;
        c = ((cursor_t *)cursor)->curs.cur_byte;
        if ((c >= 'A') && (c <= 'Z'))
            return c + 0x20;
        return c;
    case 9:	return Look_ahead(1) ? ((cursor_t *)cursor)->cursw.cur_word : 0; /* CURWD */
    case 10: return Look_ahead(1) ? ((cursor_t *)cursor)->curs.next_byte : 0;  /* NXTCH */
    case 11: return Look_ahead(3) ? ((cursor_t *)cursor)->cursw.next_word : 0;  /* NXTWD */
    case 12:
    {
        toff_t tmp = {.bp = oa.high_s}; return Get_offset(oa.wk1_blocks, tmp);
    }
    case 13:	return Get_offset(oa.tblock[1], oa.toff[1]);
    case 14:	return Get_offset(oa.tblock[2], oa.toff[2]);
    case 15:	return Get_offset(oa.tblock[3], oa.toff[3]);
    case 16:	return Get_offset(oa.tblock[4], oa.toff[4]);
    case 17:    return oa.high_s + 1 == oa.high_e && !Can_forward_file() ? ~0 : _FALSE;  /* EOF */
    case 18:	return oa.low_s == oa.low_e && !Can_backup_file() ? ~0 : _FALSE; /* BOF */
    case 19:    return Convert_time_and_date(true); /* DATE */
    case 20:	return Convert_time_and_date(false); /* TIME */
    case 21:	return indent_margin;
    case 22:	return left_margin;
    case 23:	return right_margin;
    case 24:	return macro_exec_level;
    case 25:	return cnt_fnd;
    case 26:	return cnt_rep;
    case 27:	return cnt_exe[macro_exec_level + 1];
    case 28:	return cnt_exe[macro_exec_level];
    case 29:
    { /* NEXTAB */

        pointer ch_ptr;
        byte i, len;
        word pos;

        pos = Real_pos();
        len = tab_to[pos];
        if (!Look_ahead(len))
            return 0;
        pos += tab_to[pos];
        ch_ptr = oa.high_s;
        for (ch_ptr = oa.high_s, i = 1; i <= len; i++, ch_ptr++)
            if (*ch_ptr == LF)
                return 0;
        return pos;
    }
    case 30:	return s0[0];
    case 31:	return s1[0];
    case 32:	return s2[0];
    case 33:	return s3[0];
    case 34:	return s4[0];
    case 35:	return s5[0];
    case 36:	return s6[0];
    case 37:	return s7[0];
    case 38:	return s8[0];
    case 39:	return s9[0];
    case 40:    return in_block_buffer > string_len ? 0 : in_block_buffer;
    case 41:	return in_other ? ob.input_name[0] : oa.input_name[0];
    case 42:	return in_other ? oa.input_name[0] : ob.input_name[0];
    case 43:	return s_write_file[0];
    case 44:	return s_get_file[0];
    case 45:	return s_put_file[0];
    case 46:	return s_macro_file[0];
    case 47:	return target[0];
    case 48:	return replacement[0];
    case 49:	return s_input_file[0];
    }
    return 0;         /* To satisfy SLAC */

} /* system_variable */


/* CALC ERROR MESSAGES */
byte err_illegal_expo[] = { "\x1d" "illegal exponential operation" };
byte err_unbalanced[] = { "\x16" "unbalanced parenthesis" };
byte err_too_complex[] = { "\x16" "expression too complex" };
byte err_div_by_zero[] = { "\x14" "divide by zero error" };
byte err_illegal[] = { "\x12" "illegal expression" };
byte err_mod_by_zero[] = { "\x11" "mod by zero error" };
byte err_unrecognized[] = { "\x17" "unrecognized identifier" };
byte err_illegal_num[] = { "\x18" "invalid numeric constant" };
byte err_num_too_long[] = { "\x1a" "numeric constant too large" };
byte err_invalid_base[] = { "\x16" "invalid base character" };
byte err_no_float[] = { "\x1a" "floating point not allowed" };
byte err_long_string[] = { "\x12" "string is too long" };
byte err_read_only[] = { "\x20" "assignment to read only variable" };


/*************************************************************************
Is called for calc errors. Prints an error message, and returns to
the module level (the compiler cleans up the stack).
*************************************************************************/
#ifdef MSDOS
__declspec(noreturn)
#else
__attribute__((noreturn))
#endif
    static void Calc_error(pointer err_msg) {

    byte len, len1;

    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_str(err_msg);
    Add_str_str("\x4" "    ");
    while (tok_ptr <= input_buffer + input_buffer[0] &&
        (Char_type(tok_ptr[0]) == ch_letter || Char_type(tok_ptr[0]) == ch_number)) {
        tok_ptr++;
    }
    tok_ptr[0] = '#';
    len = len1 = (byte)(tok_ptr - input_buffer);
    if (len1 > 10) len1 = 10;
    input_buffer[len - len1] = len1;
    Add_str_str(&input_buffer[len - len1]);
    Error(tmp_str);
    longjmp(aedit_entry, 1);

} /* calc_error */


/******************************************************************
Converts chars in STRING to binary value. chars must be 0..9 A..F
Base should be 2,8,10 or 16.
******************************************************************/
nestedProc dword Convert(byte base, pointer string, byte n) {
    byte i;
    dword val;
    dword maxval;
    byte c;

    val = 0;
    if (base == 10)
        maxval = 0x7fffffff / 10;
    else
        maxval = 0xffffffff / base;
    for (i = 1; i <= n; i++) {
        c = string[i];
        if (c <= '9')      /* 0..9 */
            c = c - '0';
        else if (c <= 'F') /* A..F */
            c = c - 'A' + 10;
        else
            Calc_error(err_illegal_num);

        if (c >= base)   /* OUT OF RANGE FOR THE BASE */
            Calc_error(err_illegal_num);
        if (val > maxval) /* ovf */
            Calc_error(err_num_too_long);
        val = val * base + c;
        if (base == 10 && val > 0x7fffffff /* ovf */)
            Calc_error(err_num_too_long);
    }
    return val;
} /* convert */

/*************************************************************************
*************************************************************************/
static dword Convert_number() {
    byte n, c;

    /** CONVERT_NUMBER PROC CODE **/
    n = string[0];
    c = string[n];
    if (c <= '9') { /* 0..9 */
        return Convert(10, string, n);
    }
    else {
        n--;
        if (c == 'H') {
            return Convert(16, string, n);
        }
        else if (c == 'Q' || c == 'O') {
            return Convert(8, string, n);
        }
        else if (c == 'B') {
            return Convert(2, string, n);
        }
        else if (c == 'D') {
            return Convert(10, string, n);
        }
        else {
            Calc_error(err_invalid_base);
        }
    }
    return 0;
} /* convert_number */


static dword Collect_number() {
    byte ch, i;

    ch = *tok_ptr; /* first char is surely a digit */
    i = 1;
    while ((ch >= '0') && (ch <= '9') || (ch >= 'A') && (ch <= 'Q')) {
        /* A..F are hex;  B,D,O,Q are suffix */
        string[i] = ch;
        i++;
        ch = Upper(*++tok_ptr);
    }
    if (ch == '.')     /* it had better be a floatint point number */
        Calc_error(err_no_float);
        string[0] = i - 1;
        return Convert_number();
} /* collect_number */



/*  CLCTST */







byte shift_ops[] = {
    "ROL"        /* circular left shift */
    "ROR"        /* circular right shift */
    "SHL"        /* logical left shift */
    "SHR"        /* logicial right shift */
    "SAL"        /* arithmatic left shift */
    "SAR" };       /* arithmatic right shift */

/* Token types. */

#define type_nreg	0
/* Ni */

#define type_const	1
#define type_assign	2
#define type_lpar	3
#define type_rpar	4
#define type_CR	5
#define type_plsmin	6
#define type_unary	7
#define type_expo	8
#define type_muldiv	9
#define type_shift	10
#define type_relat	11
#define type_logic	12
#define type_sreg	13
/* Si */

#define type_indexed_nreg	15
/* N(n_statement) */

#define type_indexed_sreg	16
/* S(n_statement) */


/*************************************************************************
*************************************************************************/
static void Scan() {
    pointer str_p;
    byte i, b, c;

    while (*tok_ptr == ' ' || *tok_ptr == TAB)
        tok_ptr++;

    switch (Char_type(*tok_ptr)) {

    case 0:	 /* error */
        Calc_error(err_illegal);
        break;

    case 1:	/* 0..9 */
        token = type_const;
        token_val = Collect_number();
        break;
    case 2:	 /* = */
        if (tok_ptr[1] == '=') {
            token = type_relat;
            token_subtype = 0;
            tok_ptr++;
        }
        else
            token = type_assign;
        tok_ptr++;
        break;
    case 3:	 /* < */
        token = type_relat;
        if (*++tok_ptr == '>') {
            token_subtype = 1;
            tok_ptr++;
        } else if (*tok_ptr == '=') {
            token_subtype = 5;
            tok_ptr++;
        }  else
            token_subtype = 4;
        break;
    case 4:	 /* > */
        token = type_relat;
        if (*++tok_ptr == '=') {
            token_subtype = 3;
            tok_ptr++;
        }
        else
            token_subtype = 2;
        break;
    case 5:	 /* +,- */
        token = type_plsmin;
        token_subtype = Char_subtype(*tok_ptr);
        tok_ptr++;
        break;
    case 6:	 /* ( */
        token = type_lpar;
        tok_ptr++;
        break;
    case 7:	 /* ) */
        token = type_rpar;
        tok_ptr++;
        break;
    case 8:	 /* <CR> */
        token = type_CR;
        tok_ptr++;
        break;
    case 9:	 /* ~,! */
        token = type_unary;
        token_subtype = Char_subtype(*tok_ptr++);
        break;
    case 10:	/* * */
        if (*++tok_ptr == '*') {
            token = type_expo;
            tok_ptr++;
        }
        else {
            token = type_muldiv;
            token_subtype = 0;
        }
        break;
    case 11:	 /* /,\ */
        token = type_muldiv;
        token_subtype = Char_subtype(*tok_ptr++);
        break;


    case 12:	 /* |,&,^ */
        token = type_logic;
        token_subtype = Char_subtype(*tok_ptr++);
        break;


    case 13:	break;
    case 14:	 /* A..Z */
        *tok_ptr = Upper(*tok_ptr);

        for (i = 1; Char_type(tok_ptr[i]) == ch_letter; i++)
            tok_ptr[i] = Upper(tok_ptr[i]);
        if (*tok_ptr == 'N') {
            if (Char_type(tok_ptr[1]) == ch_number && Char_type(tok_ptr[2]) != ch_letter) {
                token = type_nreg;
                token_val = tok_ptr[1] - '0'; /* return the index */
                tok_ptr += 2;
                return;
            }
            c = tok_ptr[1];
            if (c == ' ' || c == TAB || c == '(') {
                token = type_indexed_nreg;
                tok_ptr++;
                return;
            }
        }
        else if (*tok_ptr == 'S') {
            b = Find_index(tok_ptr[1], s_var_names);
            if (b < s_var_names[0] && Char_type(tok_ptr[2]) != ch_letter) {
                token = type_sreg;
                token_val = b;
                tok_ptr += 2;
                return;
            }
            c = tok_ptr[1];
            if (c == ' ' || c == TAB || c == '(') {
                token = type_indexed_sreg;
                tok_ptr++;
                return;
            }
        }

        str_p = shift_ops;
        for (i = 0; i < 6 && cmpb(tok_ptr, str_p, 3) != 0xffff; i++)
            str_p += 3;
        if (i != 6) {
            token = type_shift;
            token_subtype = i;
            tok_ptr += 3;
            return;
        }

        if (Is_a_system_variable()) {
            token = type_const;
            token_val = System_variable();
        }
        else
            Calc_error(err_unrecognized);


        break;


    }
} /* scan */





/*************************************************************************
Compare two signed dwords:  If num1 is greater - return 1;
    else if num2 is greater - return 2; else return 0;
*************************************************************************/
static byte Cmp_dw(dword num1, dword num2) {
    if (num1 == num2)
        return 0;
    sdword snum1 = (sdword)num1;
    sdword snum2 = (sdword)num2;
    return snum1 > snum2 ? 1 : 2;
}


/*************************************************************************
Save current scanner's state for backtracking.
*************************************************************************/
static void Save_state() {
    temp_tok_ptr = tok_ptr;
    temp_tok = token;
}

/*************************************************************************
Restore scanner's state after backtracking.
*************************************************************************/
static void Restore_state() {
    tok_ptr = temp_tok_ptr;
    token = temp_tok;
}


/*************************************************************************
Check if the following input is an assignment.
*************************************************************************/
static boolean Is_assign() {
    byte par_count; /* parens counter. */

    if ((token == type_nreg) || (token == type_sreg)) {
        Scan();
        return (token == type_assign);
    }
    if ((token != type_indexed_nreg) && (token != type_indexed_sreg)) return _FALSE;
    Scan();
    if (token != type_lpar) return _FALSE;
    Scan();
    par_count = 1;
    while ((token != type_CR)) {
        if (token == type_lpar) {
            par_count++;
        }
        else if (token == type_rpar) {
            par_count--;
            if (par_count == 0) {
                Scan();
                return (token == type_assign);
            }
        }
        Scan();
    }
    return false;
}



/*************************************************************************
    Check if during the recursion we have blown the stack.
NOTE:
    86:
        The check in the 86 version is valid only if the stack segment is
        the first segment in DGROUP. To force it use the control ORDER in
        the linkage: ' ORDER(STACK,CONST,DATA,MEMORY) ' .
    286:
        The stack segment size (specified in BIND invocation)
        is assumed to be 0C00H.
*************************************************************************/
static void Check_stack_ovf() {

#define Minimum_Stack_Size	0x100

    /* stack overflow check */
 //   if (stack_ptr - (get_segment_limit[stack_base] - 0xc00) < Minimum_Stack_Size) Calc_error(err_too_complex);

}




/*************************************************************************
*************************************************************************/
static pointer Get_quoted_string() {
    byte delim;
    pointer start, limit;

    /* assuming we are at the first delimiter. */
    delim = *tok_ptr;
    tok_ptr++;
    start = tok_ptr;
    limit = &input_buffer[input_buffer[0]];
    while (*tok_ptr != delim && tok_ptr <= limit) {
        tok_ptr++;
    }
    if (tok_ptr > limit)
        Calc_error("\x19" "missing string terminator"); /* does not return */
    tmp_str[0] = (byte)(tok_ptr - start);
    memcpy(&tmp_str[1], start, tmp_str[0]);
    tok_ptr++;
    Scan(); /* next token must be CR (checked by calc_statement) */
    return tmp_str;

} /* get_quoted_string */




/*************************************************************************
*************************************************************************/
static boolean Check_if_quoted_string() {
    while (*tok_ptr == ' ' || *tok_ptr == TAB)
        tok_ptr++;

    if (tok_ptr > &input_buffer[input_buffer[0]])
        return _FALSE;
    return delimiters[*tok_ptr] == 1; /* the string is quoted by delimiters. */

} /* check_if_quoted_string */







/*************************************************************************
        Usage: element := Ni
                    |  N(n_statement)
                    |  (n_statement)
                    |  numeric_constant
                    |  calc_variable
*************************************************************************/
static dword Element() {
    dword num1;

    if (token == type_nreg) {
        num1 = token_val;
        Scan(); /* skip the Ni */
        if (num1 > NMAX)
            Calc_error(err_unrecognized);
        return n_var[num1];
    }
    if (token == type_indexed_nreg) {
        Scan(); /* skip the N */
        num1 = N_stat_in_parens();
        if (num1 > NMAX)
            Calc_error(err_unrecognized);
        return n_var[num1];
    }
    if (token == type_lpar) {
        return N_stat_in_parens();
    }
    if (token == type_const) {
        num1 = token_val;
        Scan(); /* skip the const */
        return num1;
    }
    Calc_error(err_illegal);
    return 0;

} /* element */




/*************************************************************************
        Usage: primary :=  +primary |-primary | (unimp)--primary | (unimp)++primary |
                            ~primary | !primary | #primary | element
*************************************************************************/
static dword Primary() {
    dword num1;
    byte op;

    Check_stack_ovf();
    if ((token == type_plsmin) || (token == type_unary)) op = token_subtype;
    else return Element();
    Scan();
    num1 = Primary();
    switch (op) {
    case 0:	return num1;                /* + */
    case 1:	return ~num1 + 1;   // -num  /* - */
    case 2: return (num1 & 0x8000000) != 0 ? 0 : ~0;    /* ! */
    case 3:	return ~num1;               /* ~ ` */
    case 4: return (num1 & 0x8000000) != 0 ? ~0 : 0;    /* # */
    }
    return 0;
} /* primary */



/*************************************************************************
        Usage: factor :=   factor ** primary | primary
*************************************************************************/
static dword Factor() {
    dword num1, num2, val;
    boolean sign;

    Check_stack_ovf();
    num1 = Primary();
    if (token != type_expo)
        return num1;
    Scan();
    num2 = Factor();
    if ((num2 & signBit) != 0)
        Calc_error(err_illegal_expo);
    if ((num1 & signBit) != 0) {
        sign = _TRUE;
        num1 = ~num1 + 1;
    }
    else sign = _FALSE;
    val = 1;
#if 0
    word i, j;
    // PMO: this code appears to be faulty. Works ok if num2 <= 0xffff or num1 == 1
    // but as overflow will occur for num1 > 1 with num2 > 31 it is not really a big issue
    // can fail with num1 = 0 if num2 != 0 && num2 % 0x10000 == 0
    for (j = 0; j <= highw(num2); j++) { /* Instead of HIGH(num2) due to PLM/VAX bug. */
        for (i = 1; i <= loww(num2); i++)
            val = val * num1;
    }
#else
    while (num2-- > 0)
        val *= num1;
#endif
    if (sign)
        val = ~val + 1; // -val
    return val;
} /* factor */


/*************************************************************************
        Usage: term  :=  term * factor | term / factor |
                            term \ factor | factor
*************************************************************************/
static dword Term() {
    dword num1, num2;
    byte op;
    boolean sign1, sign2;

    num1 = Factor();
    while (token == type_muldiv) {
        op = token_subtype;
        Scan();
        num2 = Factor();
        sign1 = sign2 = _FALSE;
        if (num1 & signBit) {
            sign1 = _TRUE;
            num1 = ~num1 + 1;
        }
        if (num2 & signBit) {
            sign2 = _TRUE;
            num2 = ~num2 + 1;
        }
        switch (op) {
        case 0: /* * */
            num1 = num1 * num2;

                break;

        case 1: /* / */
            if (num2 == 0)
                Calc_error(err_div_by_zero);
            num1 = num1 / num2;
                break;

        case 2:  /* \ */
            if (num2 == 0)
                Calc_error(err_mod_by_zero);
            sign2 = _FALSE;
            num1 = num1 % num2;
                break;
        }
        if (sign1 ^ sign2)
            num1 = ~num1 + 1;
    }
    return num1;
} /* term */


/*************************************************************************
        Usage: exp  :=  exp + term | exp - term | term
*************************************************************************/
static dword Exp() {
    dword num1, num2;
    byte op;

    num1 = Term();
    while (token == type_plsmin) {
        op = token_subtype;
        Scan();
        num2 = Term();
        switch (op) {
        case 0:	num1 = num1 + num2;
            break;

        case 1:	num1 = num1 - num2;
            break;

        }
    }
    return num1;
} /* exp */


/*************************************************************************
        Usage: srops :=  srops $CLS$ exp |  srops $CRS$ exp |
                        srops $LLS$ exp |  srops $LRS$ exp |
                        srops $ALS$ exp |  srops $ARS$ exp |
                        exp
*************************************************************************/
static dword Srops() {

    dword num1; byte num2;
    byte op;

    num1 = Exp();
    while (token == type_shift) {
        op = token_subtype;
        Scan();
        num2 = Exp();           /* The rotation is modulu 256 */
        switch (op) {
        case 0:	num2 %= nbits;
            num1 = (num1 << num2) + (num1 >> (nbits - num2)); // rol(num1, num2);
            break;
        case 1:	num2 %= nbits;
            num1 = (num1 >> num2) + (num1 << (nbits - num2)); // ror(num1, num2);
            break;
        case 2:	num1 <<= num2;
            break;
        case 3:
            num1 >>= num2;
                break;
        case 4:
            num1 = (num1 & signBit) | (num1 << num2);
            break;

        case 5:
            num1 = (dword)(((sdword)num1) >> num2);      // rely on c behaviour for signed shr
            break;
        }
    }
    return num1;
} /* srops */


/*************************************************************************
        Usage: relops  :=  relops == srops  |  relops <> srops
                            | relops <= srops  |  relops >= srops
                            | relops  < srops  |  relops  > srops
                            | srops
*************************************************************************/
static dword Relops() {
    dword num1, num2;
    byte op; bool b;

    num1 = Srops();
    while (token == type_relat) {
        op = token_subtype;
        Scan();
        num2 = Srops();
        b = Cmp_dw(num1, num2);
        switch (op) {
        case 0:	b = b == 0;
            break;

        case 1:	b = b != 0;
            break;

        case 2:	b = b == 1;
            break;

        case 3:	b = (b == 1) || (b == 0);
            break;

        case 4:	b = b == 2;
            break;

        case 5:	b = (b == 2) || (b == 0);
            break;

        }
        num1 = b ? 0xffffffff : 0;
    }
    return num1;
} /* relops */


/*************************************************************************
        Usage: logicalops := logicalops '|' relops | logcialops & relops |
                                logicalops ^ relops   |  relops
*************************************************************************/
static dword Logicalops() {
    dword num1, num2;
    byte op;

    num1 = Relops();
    while (token == type_logic) {
        op = token_subtype;
        Scan();
        num2 = Relops();
        switch (op) {
        case 0:	num1 = num1 | num2;
            break;

        case 1:	num1 = num1 & num2;
            break;

        case 2:	num1 = num1 ^ num2;
            break;

        }
    }
    return num1;
} /* logicalops */


/*************************************************************************
        Usage: n_statement := Ni             = n_statement
                            |  N(n_statement) = n_statement
                            |  logicalops
*************************************************************************/
static dword N_statement() {

    dword index;
    boolean b;

    Check_stack_ovf();
    Save_state();
    b = Is_assign();
    Restore_state();
    if (b) {
        if (force == 0        /* unknown yet */) force = 1; /* don't print message */
        if (token == type_nreg) {
            index = token_val;
            Scan(); /* skip the Ni */
        }
        else {
            Scan(); /* skip the N */
            index = N_stat_in_parens();
        }
        if (index > NMAX)
            Calc_error(err_unrecognized);
        Scan(); /* skip the = */
        n_var[index] = N_statement();
        return n_var[index];
    }
    else {
        if (force == 0        /* unknown yet */) force = 2; /* print message */
        return Logicalops();
    }
} /* n_statement */




/*************************************************************************
*************************************************************************/
static dword N_stat_in_parens() {

    dword num1;
    if (token != type_lpar)
        Calc_error(err_illegal);    /* does not return */
    Scan();
    num1 = N_statement();
    if (token != type_rpar)
        Calc_error(err_unbalanced);    /* does not return */
    Scan();
    return num1;

} /* n_stat_in_parens */




/*************************************************************************
        Usage: S_statement :=  Si             = S_statement
                            |   S(n_statement) = S_statement
                            |   S(n_statement) = '<string>[']
                            |   S(n_statement)
*************************************************************************/
static pointer S_statement() {
    dword index;
    pointer str_p;
    byte c, tmp;

    ;
    Check_stack_ovf();
    if (token == type_sreg) {
        index = token_val;
        c = s_var_names[index + 1]; /* second char of Si */
        Scan(); /* skip the S or Si*/
    }
    else {
        Scan(); /* skip the S or Si*/
        tmp = force;
        force = 3; /* ignore it temporarily when calculating the index */
        index = N_stat_in_parens();
        force = tmp; /* restore its value */
        if (index > 9)
            Calc_error(err_unrecognized);
        c = index + '0'; /* need a char, not an index. */
    }
    if (token == type_assign) {
        force = 1; /* don't print */
        /* L_value string variable. */
        if (index > 9)
            Calc_error(err_read_only);
        if (Check_if_quoted_string()) {
            ;
            str_p = Get_quoted_string();
        }
        else {
            Scan(); /* skip the = */
            str_p = S_statement();
        }
        Move_name(str_p, s_var[index]);
        str_p = s_var[index];
    }
    else {
        if (force == 0        /* unknown yet */) force = 2; /* print message */
        /* R-value string variable. */
        str_p = Get_string_variable(c);
        if (string_error)
            longjmp(aedit_entry, 1);
    }
    return str_p; /* In both cases it has R-value. */
} /* s_statement */




dword result;
byte result_type;
pointer result_ptr;
#define numeric_type	0
#define string_type	1




/*************************************************************************
        Usage: calc_statement := n_statement | s_statement
*************************************************************************/
static void Calc_statement() {

    Scan();
    if (token == type_CR)
        longjmp(aedit_entry, 1);

    if ((token == type_sreg) || (token == type_indexed_sreg)) {
        result_type = string_type;
        result_ptr = S_statement();
    }
    else {
        result_type = numeric_type;
        result = N_statement();
    }
    if (token != type_CR)
        Calc_error(err_illegal);
} /* calc_statement */




void C_cmnd() {

    if (Input_line("\x6" "Calc: ", s_calc) == CONTROLC)
        return;

    /* Store the input for next reediting. */
    Move_name(input_buffer, s_calc);
    input_buffer[input_buffer[0] + 1] = CR;
    force = 0; /* unknown yet */

    Working();

    /* Do the calculation. */
    tok_ptr = &input_buffer[1];
    Calc_statement();

    /* Print results */
    Init_str(tmp_str, sizeof(tmp_str));
    Add_str_char(' ');
    if (result_type == numeric_type) {
        if ((result & signBit) != 0) {
            Add_str_char('-');
            Add_str_num(~result + 1, 10);
        }
        else {
            Add_str_num(result, 10);
        }
        Add_str_str("\x2" "  ");
        Add_str_num(result, 16);
        Add_str_char('H');
    }
    else {
        Add_str_str("\x5" "     ");
        /* the message should be longer than 5,
            otherwise it will not be printed in batch mode */
        Add_str_special(result_ptr);
    }
    force_writing = (force == 2); /* 2 - print */
    Print_message(tmp_str);
    if (macro_exec_level != 0) Co_flush();
    force_writing = _FALSE;

} /* c_cmnd */

