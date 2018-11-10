// #SMALL
// #title('UTIL               UTILITY ROUTINES')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

#include <stdbool.h>
#include <string.h>
#include <memory.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"

// Min & Max replaced by macros to support different number types

word Size_of_text_in_memory() {
    return oa.high_e - oa.high_s + oa.low_e - oa.low_s;
} /* size_of_text_in_memory */


/*
    MOVE A NAME
*/

void Move_name(pointer fromp, pointer top) {

    memcpy(top, fromp, *fromp + 1);
} /* move_name */




/*
    COMPARE TWO STRINGS
*/
byte Cmp_name(pointer fromp, pointer top) {
    return memcmp(fromp, top, *fromp + 1) == 0 ? _TRUE : _FALSE;
} /* cmp_name */





byte outfield[34];

/***************************************************************************
  Prints a 32 bit number. BASE is the required base. B$WIDTH is the
  printing width. If the number is greater than 07FFF$FFFFH, then only
  the bases 2,8,16 are allowed (due to PLM/VAX malfunction).
  If b$width is negative, the number is left justified; otherwise -
  right justified. If base is negative, the filling chars are zeroes,
  otherwise spaces.
  Returns pointer to the buffer in which the number is expanded.
***************************************************************************/
pointer Print_number(dword d_number, byte b_width, byte base) {

    byte hexdigits[16] = "0123456789ABCDEF";
    byte field[34];       /* FIELD[0] IS UNUSED */
    byte fill_char;
    short i, places, width;

    //    Fill_chars expanded in line

    outfield[0] = 0;
    width = (signed char)b_width;

    fill_char = ' ';                    /* initial fill char to blank */
    if (base > 127) {        /* test for 0 fill char */
        base = -base;
        fill_char = '0';
    }
    if (base > 16)    /* make sure base is */
        base = 16;            /* in range */
    else if (base < 2) {
        base = 10;
    }
    memset(field, fill_char, sizeof(field));      /* initialize field */
    i = sizeof(field);

    while (d_number != 0) {     // note original did not check for decimal numbers with top bit set so this doesn't
        field[--i] = hexdigits[d_number % base];
        d_number /= base;
    }
    if (i == sizeof(field))
        field[--i] = '0';

    places = sizeof(field) - i;
    for (i = places + 1; i <= width; i++)     /* print out leading fill chars */
        outfield[++outfield[0]] = fill_char;

    for (i = sizeof(field) - places; i < sizeof(field); i++) {        /* print out number */
        outfield[++outfield[0]] = field[i];
    }
    if (width < 0) {            /* print out post fill chars */
        width = -width;
        for (i = places + 1; i <= width; i++)    /* print out leading fill chars */
            outfield[++outfield[0]] = fill_char;

    }
    return outfield;

} /* print_number */


pointer string_p;
byte max_string_length;


void Init_str(pointer str_p, byte len) {
    string_p = str_p;
    max_string_length = len - 1; /* Substract leading byte. */
    string_p[0] = 0;
} /* init_str */



void Reuse_str(pointer str_p, byte len) {
    /* Initialize, but do not cancel current contents. */

    string_p = str_p;
    max_string_length = len - 1; /* Substract leading byte. */
} /* reuse_str */



void Add_str_char(byte ch) {

    if (string_p[0] >= max_string_length)
        return;
    string_p[++string_p[0]] = ch;
} /* add_str_char */




void Add_str_str(pointer str_p) {

    for (byte i = str_p[0]; i > 0; i--)
        Add_str_char(*++str_p);
} /* add_str_str */




void Add_str_num(dword num, byte base) {
    Add_str_str(Print_number(num, 0, base));
} /* add_str_num */




/********************************************************
 Utilities for input-line scanning.
********************************************************/

pointer scan_p;


void Init_scan(pointer str_p) {
    scan_p = str_p;
} /* init_scan */



void Swb() {
    while (*scan_p == ' ' || *scan_p == TAB) {
        scan_p++;
    }
} /* swb */



boolean Skip_char(byte ch) {
    Swb();
    if (*scan_p != ch)
        return _FALSE;
    scan_p++;
    return _TRUE;
} /* skip_char */



word Num_in(boolean *err_p) {
    byte ch;
    dword num;

    *err_p = _FALSE;
    Swb();
    if (*scan_p < '0' || *scan_p > '9') {
        *err_p = _TRUE;
        return 0;
    }
    num = 0;
    while (1) {
        ch = *scan_p - '0';
        if (ch < 0 || ch > 9)
            return num;
        num = num * 10 + ch;
        if ((num >> 16) != 0) {
            *err_p = _TRUE;
            return 0;
        }
        scan_p++;
    }
} /* num_in */


/********************************************************/

