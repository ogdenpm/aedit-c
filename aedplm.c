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

/************************************************************
    PLACE THE  CURSOR SOMEWHERE IN  THE TEXT.   NO ERRORS ARE
    POSSIBLE.  IF FORCE  COLUMN  IS  LESS  THAN  80  THEN  IT
    SPECIFIES  THE COLUMN  THAT MUST  BE MOVED  TO.   END THE
    ROUTINE WITH A PUT_GOTO TO SET THE CURSOR.
************************************************************/
void Position_in_text() {
    word row, tcol;
    pointer addr;

    /*    CALCULATE ROW BY COMPARING ADDRESSES IN HAVE ARRAY WITH HIGH_S */

    row = first_text_line;
    while (have[row] < oa.high_s) {
        if (++row > message_line) {
            row = message_line;
            force_column = 0;
            goto ex_loop;
        }
    }
    if (have[row] > oa.high_s)
        row--;
ex_loop:
    addr = have[row];
    oa.bol = addr;

    if (force_column >= 80) {
        tcol = 0;
        while (tcol < wrapping_column && addr < oa.low_e) {
            if (*addr == TAB)
                tcol += tab_to[tcol];
            else
                tcol++;
            addr++;
        }

        virtual_col = (byte)tcol;
        if (oa.left_column != 0) {
            if (oa.left_column <= tcol) /* adjust for left column */
                tcol = tcol - oa.left_column;
            else
                tcol = 0;
        }
        if (tcol > 79)
            tcol = 79;
    }
    else
        tcol = force_column;

    force_column = 100;    /* for next time */

    Put_goto((byte)tcol, (byte)row);

    cursor = oa.high_s;

} /* Position_in_text() */





/*      Translated from ASM86    -     IB  */

pointer Unfold(pointer textp, pointer image_p) {
    byte ch, count, image_index, actual_col, l_col;

    low_e_byte = LF;
    if (macro_suppress && !force_writing) {
        image_p[0] = 0;

        /*      search_for_lf: */
        while (1) {
            textp += findb(textp, LF, wrapping_column - 1);
            if (textp == oa.low_e) {
                textp = oa.high_s;
                continue;
            }
            if (textp >= oa.high_e) {
                if (Can_forward_file() != 0) {
                    Set_tag(ed_tagb, oa.high_s);
                    Forward_file();
                    textp = oa.high_s;
                    Jump_tag(ed_tagb);
                    continue;
                }
                return oa.high_e;
            }
            if (textp[0] == LF)
                textp++;
            if (textp == oa.low_e)
                textp = oa.high_s;
            return textp;
        }
    }

    image_index = 0;
    l_col = oa.left_column;
    actual_col = 0;
    if (oa.left_column != 0) {
        image_index = 1;
        image_p[1] = '!';
        l_col++;
    }

    /* unfold_loop: */
    while (1) {
        ch = Printable(textp[0]);
        textp++;
        if (ch != TAB && ch != LF) {
            if (++actual_col == wrapping_column)
                goto lf_case;
            if (actual_col <= l_col)
                continue;
            image_index++;
            if (image_index > 80) {
                image_index--;
                ch = '!';
            }
            image_p[image_index] = ch;
            continue;
        }
        if (ch == LF) goto lf_case;
        count = tab_to[actual_col];
        ch = print_as[' '];
        /*      tab_loop: */
        while (count != 0) {
            count--;
            if (++actual_col == wrapping_column)
                goto lf_case;
            if (actual_col <= l_col)
                continue;
            if (++image_index > 80) {
                image_index--;
                ch = '!';
            }
            image_p[image_index] = ch;
        }
        continue;
    lf_case:
        if (--textp == oa.low_e) {
            textp = oa.high_s;
            continue;
        }
        if (textp >= oa.high_e) {
            if (Can_forward_file() != 0) {
                Set_tag(ed_tagb, oa.high_s);
                Forward_file();
                textp = oa.high_s;
                Jump_tag(ed_tagb);
                continue;
            }
            if (actual_col == 0)
                image_index = 0;
            image_p[0] = image_index;
            return oa.high_e;
        }
        if (ch != LF && actual_col == wrapping_column) {
            image_index++;
            if (image_index >= 80)
                image_index--;
            image_p[image_index] = '+';
        }
        image_p[0] = image_index;
        if (textp[0] == LF)
            textp++;
        if (textp == oa.low_e)
            textp = oa.high_s;
        return textp;
    }
} /* Unfold */



/*
;    NEXT_LINE            GIVEN A POINTER, FIND THE NEXT
;                    LF AND RETURN POINTER TO FIRST
;                    CHARACTER PAST LF. DO NOT
;                    WORRY ABOUT WINDOW, END OF TEXT
;                    ETC.
*/

pointer Next_line(pointer str) {
    word tmp;

    tmp = findb(str, LF, wrapping_column);
    if (tmp != 0xffff)
        return str + (tmp & 0xff) + 1;
    return str + wrapping_column;

} /* Next_line */





/*
;    PRIOR_LINE            GIVEN A POINTER, SUBTRACT 1 AND THEN
;                    FIND THE PRIOR LF AND RETURN POINTER
;                    TO LF. DO NOT WORRY ABOUT WINDOW,
;                    END OF TEXT ETC.
*/
pointer Prior_line(pointer str) {
    word tmp;

    str -= wrapping_column;
    tmp = findrb(str, LF, wrapping_column);
    if (tmp != 0xffff)
        return str + (tmp & 0xff);
    return str;
} /* Prior_line */



/* Cc_trap() is in separate module so we can get long calls */


