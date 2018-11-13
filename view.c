// #SMALL
// #title('VIEW                     REBUILD THE SCREEN DISPLAY')
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


/*        LOCAL VARIABLES  */

byte saved_line = {_FALSE};
byte psaved_line = {_FALSE};
byte saved_row;            /* ROW NUMBER OF SAVED LINE */



void Movmem(pointer from_word, pointer to_word, word len) {
    pointer last_moved;
    int distance;           // can be negative if to < from
    wbyte i;
    byte dl, dh;
    byte got_both_windows;
    pointer saver;

    memmove(to_word, from_word, len);

    /*    CORRECT TAGS IF NECESSARY    */

    saver = oa.toff[ed_tagw];

    last_moved = from_word + len - 1;
    distance = (int)(to_word - from_word);
    for (i = 1; i <= num_tag; i++) {
        if (oa.tblock[i] == oa.wk1_blocks && oa.toff[i] >= from_word
            && oa.toff[i] <= last_moved)
            oa.toff[i] += distance;
    }

    /* PRESEVE the setting of ed_tagw if we have called clean tags
       while not in the same file that that tag is reserved for */
    if (in_other != w_in_other)
        oa.toff[ed_tagw] = saver;


    /*    AND THE POINTER TO THE SAVED LINE IMAGE    */

    if (saved_from >= from_word && saved_from <= last_moved)
        saved_from += distance;



    dl = 0;
    dh = message_line;
    got_both_windows = 0;
    if (window_present && window_num != 0)
        dh = window.message_line;

    // update_screen_pointers:
    while (1) {
        if (dl <= dh) {
            if (have[dl] < from_word) {
                dl++;
                continue;
            } else if (have[dl] <= last_moved) {
                if (have[dl] == oa.bol && !window_present || window_num == (got_both_windows & 1))
                    oa.bol += distance;
                have[dl] += distance;
                dl++;
                continue;
            }
        }
        dl = 0;
        if (!window_present || got_both_windows != 0)
            return;
        got_both_windows = 0xff;
        if (window_num) {
            dl = first_text_line;
            dh = message_line;
        } else {
            dl = window.first_text_line;
            dh = window.message_line;
        }
    }

} /* movmem */






/*
    RE_WINDOW                MOVE WINDOW SO THAT HIGH_S IS SET TO CURSOR.
                            MOVMEM WILL PRESERVE TAGS AND HAVE POINTERS
*/
void Re_window() {
    word len;

    if (cursor == oa.low_e)
        cursor = oa.high_s;
    if (cursor == oa.high_s)
        return;
    if (cursor < oa.low_e) {            /* MOVE THE WINDOW UP */
        len = (word)(oa.low_e - cursor);
        oa.high_s = oa.high_s - len;
        Movmem(cursor, oa.high_s, len);
        oa.low_e = cursor;
    } else {                            /* MOVE THE WINDOW DOWN */
        len = (word)(cursor - oa.high_s);
        Movmem(oa.high_s, oa.low_e, len);
        oa.high_s = cursor;
        oa.low_e = oa.low_e + len;
    }
    Check_window(0);
    cursor = oa.high_s;
} /* re_window */



/*
    REBUILD                    GIVEN A ROW NUMBER, NEW CONTENTS AND OLD CONTENTS,
                            TRY TO REBUILD THE LINE WITH MINIMUM FLASHING
                            AND MINIMUM CHARACTERS OUTPUT.
*/
void Rebuild(byte r_row, pointer newp, pointer oldp) {
    byte i, old_len, new_len;
    byte line_cleared;
    byte ox, oy;

#define kludge	(i < 6)


    /*    FIRST, RECORD NEW LENGTH    */

    line_cleared = _FALSE;
    oy = r_row;
    /* if we have character attributes, we must replot the whole line
       so we get the reverse video effect
    */
    new_len = newp[0];
    old_len = oldp[0];

    /*    SECOND, FIND LAST CHARACTER OF STRINGS THAT ARE THE SAME */

    i = (byte)cmpb(&newp[1], &oldp[1], old_len);
    if (i > old_len)
        i = old_len;
    if (i > new_len)
        i = new_len;

    /*    THIRD, IF THE LONGER STRING HAS TRAILING BLANKS THEN CHOP THEM OFF */

    if (r_row != prompt_line) {
        while (new_len > old_len && newp[new_len] == ' ') {
            new_len--;
        }
        while (new_len < old_len && oldp[old_len] == ' ') {
            old_len--;
        }
    }

    /*    FOURTH, IF NO CHANGE THEN ALL DONE */

    if (i != new_len || i != old_len) {

        /*    FIFTH, IF NO CLEAR REST OF LINE AND NEW LINE LENGTH IS LESS THAN
            HALF OF OLD LINE LENGTH, CLEAR ENTIRE LINE */

        if (output_codes[erase_line_out_code].code[0] == 0 &&
            new_len < old_len &&
            new_len < (old_len - new_len) &&
            output_codes[erase_entire_line_out_code].code[0] > 0
            && r_row != 0xff) {
            Put_start_row(r_row);
            Put_erase_entire_line();
            line_cleared = _TRUE;
            i = old_len = 0;
        }

        /*    SIXTH,    FORGET ABOUT THE END OF THE STRINGS IF THEY ARE IDENTICAL */

        if (new_len == old_len)
            while (newp[new_len] == oldp[new_len] && new_len > 0) {
                old_len = new_len--;
            }

        /* if we have a new prompt then reprint the character attr. */
        if (r_row == prompt_line && character_attributes
            && output_codes[reverse_video_code].code[0] > 0
            && kludge) i = 0;        /* kludge = hopefully no menues will have
                                            the first 6 characters the same
                                        */

                                        /*    SEVENTH, GOTO FIRST PLACE TO CHANGE */

        if (r_row != 0xff)
            Put_goto(i, r_row);

        /*    EIGHTH, IF NEW STRING HAS CHARACTERS LEFT, THEN WRITE THEM */

        if (i < new_len) {

            boolean no_region;
            no_region = (row == last_text_line && config == VT100);
            if (no_region)
                Reset_scroll_region();
            Print_update(i, new_len - i, &newp[i + 1], newp[1] == rvid);
            col = i = new_len;
            if (no_region)
                Put_scroll_region(first_text_line, last_text_line);
        }


        /*    NINTH, BLANK REST OF LINE IF REQUIRED    */

        if (i < old_len) {
            line_cleared = _TRUE;
            Put_erase_line();
        }

        if (line_cleared && oy == prompt_line) {
            /* if we are on a prompt line, then we must make sure we have
               the normal video byte at the end of the line if necessary
            */
            /* FIRST FIND IF WE ARE ON THE BOTTOM PROMPT LINE */
            i = prompt_line;
            if (i < window.prompt_line)
                i = window.prompt_line;

            /* IF WE ARE ON A PROMPT LINE THAT IS NOT THE BOTTOM ONE,
               OR IF WE HAVE INVISIBLE_ATTRIBUTES, THEN PLOT
               THE NORMAL VIDEO CODE (IF ANY) AT THE END OF THE LINE
            */
            if ((oy != i || ~visible_attributes)
                && output_codes[normal_video_code].code[0] > 0 && oy == prompt_line) {
                /* IF COL=80 THEN ADJUST FOR BEING OFF THE RIGHT OF THE SCREEN */
                Adjust_for_column_80();
                ox = col;
                /* FINALLY AFTER ROW AND COL ARE STRAIGHT, DO A PUT_GOTO */
                Put_normal_video_end();
                Put_goto(ox, oy);
            }
        }

        /*    TENTH, RESET ROW AND COL AND SET NEW LINE LENGTH    */

        if (r_row != 0xff)
            line_size[r_row] = newp[0];

        Adjust_for_column_80();
    }
} /* rebuild */




/*
    SAVE_LINE                    STUFF AWAY THE INDICATED LINE AS IT IS LIKELY
                                TO BE UPDATED.
*/
void Save_line(byte s_row) {

    if (psaved_line == _TRUE)
        return;

    saved_row = s_row;
    /*    IF IMPOSSIBLE THEN FLAG AS REBUILD SCREEN FROM SCRATCH */
    if (s_row > last_text_line)
        have[first_text_line] = 0;
    else {
        if (have[s_row] == oa.high_s)
            have[s_row] = oa.low_e;
        saved_from = have[s_row];
        Unfold(have[s_row], saved_bytes);
        saved_line = _TRUE;
    }
} /* save_line */




/*
    UNSAVE_LINE                UPDATE THE SAVED LINE
*/
static pointer Unsave_line(byte u_row) {
    byte unsaved_bytes[81];
    pointer next_ln;

    next_ln = Unfold(saved_from, unsaved_bytes);
    Rebuild(u_row, unsaved_bytes, saved_bytes);
    if (saved_from == oa.low_e)
        saved_from = oa.high_s;    /* RESET HAVE  */
    have[u_row] = saved_from;
    psaved_line = saved_line = _FALSE;
    return next_ln;
} /* unsave_line */




/*
    FORWARD_CHAR                FORWARD THE WINDOW UP THE INDICATED NUMBER OF
                                CHARACTERS. CAUTION: SIMILAR TO FORWARD_LINE.
                                OPTIONALLY DELETES SKIPPED CHARACTERS.
*/
void Forward_char(word num_chars, byte delete_flag) {

    pointer last_cursor;
    byte i, j;

    while ((1)) {
        if (*++cursor == LF) {
            if (cursor >= oa.high_e) {    /* RAN OUT OF TEXT */
                if (Can_forward_file()) { /* AND CAN FORWARD */
                    if (delete_flag) {    /* DELETE CURRENT STUFF */
                        oa.high_s = oa.high_e;
                        Clean_tags();
                    }
                    last_cursor = cursor - 1;
                    j = *last_cursor;
                    i = Forward_file();
                    if (*cursor == LF && j == CR)
                        num_chars++;
                } else {                    /* AT ACTUAL EOF */
                    cursor = oa.high_e - 1;
                    if (delete_flag) {
                        oa.high_s = cursor;
                        Clean_tags();
                    } else Re_window();
                    Stop_macro();
                    return;
                }

            } else { /* NORMAL LF - DO NOT COUNT IT AS A CHAR IF AFTER CR */
                last_cursor = cursor - 1;
                if (*last_cursor == CR)
                    num_chars++;
            }
        }

        num_chars--;
        if (num_chars == 0) {    /*BACKED UP INDICATED NUMBER OF CHARS*/
            if (delete_flag) {
                oa.high_s = cursor;
                Clean_tags();
            } else {
                Re_window();
            }
            return;
        }
    }

} /* forward_char */



/*
    FORWARD_LINE                MOVE THE WINDOW DOWN THE INDICATED NUMBER OF
                                LINES. FORWARD_LINE(0) IS IMPOSSIBLE.
                                FORWARD ALWAYS LEAVES WINDOW AT START OF A LINE.
*/
void Forward_line(word num_lines) {

    if (num_lines == 0)
        return;
    while ((1)) {
        cursor = Next_line(cursor);
        if (cursor >= oa.high_e) {    /* RAN OUT OF TEXT */
            /* DO NOT COUNT IF HAVE NOT FOUND A USER LF */
            if (cursor > oa.high_e)
                num_lines++;
            if (!Forward_file()) { /* AND CANNOT FORWARD */
                cursor = oa.high_e - 1;
                Re_window();
                Stop_macro();
                return;
            }
        }
        if (--num_lines == 0) {    /*BACKED UP INDICATED NUMBER OF LINES*/
            Re_window();
            return;
        }
    }
} /* forward_line */




/*
    BACKUP_CHAR                    BACK THE WINDOW UP THE INDICATED NUMBER OF
                                CHARACTERS. CAUTION: SIMILAR TO BACKUP_LINE.
                                OPTIONALLY DELETES AS IT GOES.
*/
void Backup_char(word num_chars, byte delete_flag) {
    byte i;
    logical two_rows;

    cursor = oa.low_e;                /* FIRST BYTE BEFORE WINDOW */
    while ((1)) {
        cursor--;
        /* checks against have(row) are specifically for long lines cases */
        if (oa.high_s == have[row] || oa.low_e == have[row] || *cursor == LF) {
            two_rows = _FALSE; /* should only be true when at the
                               beginning of a line on the screen */
            if (*cursor == LF) {
                if (row > first_text_line && cursor == have[row - 1])
                    two_rows = _TRUE;
                /* NAKED LF CASE */
                if (cursor[-1] == CR)
                    cursor--;
            }
            if (cursor < oa.low_s) {        /* RAN OUT OF TEXT */
                cursor = oa.low_s;
                if (delete_flag) {
                    oa.low_e = cursor;
                    Clean_tags();
                }
                if (Can_backup_file()) { /* AND CAN BACKUP */
                    num_chars++;
                    i = Backup_file();
                } else {                    /* AT START OF THE FILE */
                    if (delete_flag == _FALSE)
                        Re_window();
                    Stop_macro();
                    return;
                }
            } else if (delete_flag) {
                if (two_rows == _TRUE && saved_row > first_text_line + 1)
                    Save_line(saved_row - 2);
                else if (saved_row > first_text_line)
                    Save_line(saved_row - 1);
                else
                    V_cmnd();
            }
        }
        if (--num_chars == 0) {    /*BACKED UP INDICATED NUMBER OF CHARS*/
            if (delete_flag) {
                oa.low_e = cursor;
                Clean_tags();
            } else
                Re_window();
            return;
        }
    }
} /* backup_char */


/*
    BACKUP_LINE                        FIND AND RETURN THE START OF THE
                                    N TH LINE PRIOR TO THE SUPPLIED ADDRESS.
                                    DO_MOVE IS TRUE IF RE_WINDOW IS NEEDED
                                    AT END OF THE MOVE. NOTICE THAT IF
                                    DO_MOVE IS FALSE, NUM_LINES CAN ONLY
                                    BE A 'SMALL' VALUE. THIS IS NOT A
                                    REAL PROBLEM AS THIS ROUTINE IS ONLY
                                    CALLED WITH A LARGE VALUE IN THE 'UP'
                                    COMMAND.
                                    IF START=0 BACKUP FROM CURRENT POSITION.
*/
// original code was flawed for long lines
// on the forward pass the line is wrapped every wrap_column columns, with the last line
// showing the residual characters
// the code here shows the text by wrapping from the last rather than the first column

pointer Backup_line(int num_lines, pointer start, byte do_move) {

    pointer patch;

    /*    REVERSE SCROLL MUST BACKUP ACROSS WINDOW */
    /*    DROP A LF SO WINDOW CAN BE SKIPPED    */

    patch = oa.high_s - 1;
    *patch = LF;

    if (start == 0) {
        if (oa.bol != 0) {
            start = oa.bol;
            if (start == oa.high_s)
                start = oa.low_e;
        } else
            start = Prior_line(start);

        if (start[-1] == LF)        // include the previous LF if at start of line
            start--;
    } else {
        if (start == patch)
            start = oa.low_e;
        start = Prior_line(start);
    }

    while (1) {
        if (start == patch)
            start = oa.low_e;
        else if (start < oa.low_s) {    /* RAN OUT OF TEXT */
            if (do_move == _FALSE)  /* NEED TO REMEMBER CURRENT LOCATION */
                Set_tag(ed_tagb, oa.high_s);
            if (!Backup_file()) {/* AND CANNOT BACKUP */
                if (do_move && num_lines != 0)
                    Stop_macro();
                oa.bol = start = oa.low_s;
                break;;
            } else {
                start = oa.low_e;
                if (do_move == _FALSE)        /* IF NOT MOVING THEN PRESERVE
                                                THE CURSOR LOCATION */
                    Jump_tag(ed_tagb);
            }
        } else {
            if (num_lines == 0) {  /* BACKED UP INDICATED NUMBER OF LINES */
                if (*start == LF && start != oa.low_e)
                    start++;
                break;
            }
            num_lines--;
        }
        start = Prior_line(start);
    }


    if (do_move) {
        cursor = start;
        Re_window();
        start = oa.high_s;
    }
    if (start == oa.low_e)
        start = oa.high_s;
    return start;
} /* backup_line */




/*
    COUNT_LINES                            COUNT THE LINE FEEDS BETWEEN THE TWO
                                        POINTERS. ONLY CALLED TO DETERMINE IF
                                        SCROLLING IS POSSIBLE - NOT A GENERAL
                                        PURPOSE ROUTINE. DOES NOT WORRY
                                        ABOUT WINDOW AND STOPS WHEN COUNT
                                        REACHES PROMPT_LINE NUMBER OF LINES
*/

static byte Count_lines(pointer fromp, pointer top) {
    wbyte i;

    for (i = 0; i <= prompt_line; i++) {
        fromp = Next_line(fromp);            /* FIND THE NEXT LF+1 */
        if (fromp > top)
            return (byte)i;
    }
    return (byte)i;
} /* count_lines */





/*
    PRINT_TEXT_LINE                        UNFOLD THE TEXT INTO IMAGE
                                        THEN PRINT WITH PRINT_LINE
*/
pointer Print_text_line(pointer line) {

    pointer next_ln;
    byte image[81];
    if (config == SIIIE) {
        if (!macro_suppress || force_writing) {
            return Unfold_to_screen(line);
        }
    }
    next_ln = Unfold(line, image);
    if (!macro_suppress || force_writing)
        Print_line(image);
    return next_ln;
} /* print_text_line */


// the original PLM code appears to have been a primative port from asm
// count, SI and mostly BX are used as scaled indexes to words, so the PLM was littered with / 2
// the code changes below clean this up a little


void Re_write(pointer start) {

    word temp, count;
    byte dx;

    count = 1;

    for (dx = first_text_line; dx <= last_text_line && count + dx - 1 <= last_text_line; dx++) {
        if (start == saved_from && saved_line != 0) {
            if (saved_from >= oa.low_e && saved_from < oa.high_s)
                have[dx] = oa.high_s;
            else
                have[dx] = saved_from;
            start = Unsave_line(dx);
        } else if (have[count + dx - 1] == start)
            start = have[count + dx];
        else {
            have[dx] = start;
            if (!macro_suppress || force_writing)
                Put_start_row(dx);

            if (output_codes[delete_out_code].code[0] != 0) {
                if (!(dx >= last_text_line || (macro_suppress && !force_writing))) {
                    if ((temp = findp(&have[dx + 1], start, last_text_line - dx)) != 0xffff) {
                        if (++temp < 7 || config != SIIIE) {
                            Put_delete_line((byte)temp);
                            count += temp - 1;
                            start = have[temp + dx];
                            continue;
                        }
                    }
                }
            }

            isa_text_line = 0xff;
            start = Print_text_line(start);
        }
    }
    have[message_line] = start;
    saved_from = 0;

} /* re_write */









/*
    SCROLL                SCROLL THE SCREEN
*/

static boolean Scroll(byte num_line) {

    byte first_of_new;
    wbyte i;
    pointer next_start;
    wbyte count;
    boolean clean_scroll;
    clean_scroll = (output_codes[delete_out_code].code[0] != 0 &&
                    output_codes[reverse_scroll_out_code].code[0] != 0)
        ;

    if (window_present && !clean_scroll)
        return _FALSE;

    /* AN INSERTED CR COULD HAVE CAUSED SCROLL. REBUILD LAST LINE IF NECESSARY */
    if (have[last_text_line] == saved_from) {
        next_start = Unsave_line(last_text_line);
        saved_from = 0;
    } else {
        next_start = have[message_line];
    }

    first_of_new = message_line - num_line;

    if (config != SIV && !window_present)
        /* In aedit-86 2.0 it was     : AND NOT clean_scroll instead of : AND NOT window_present=TRUE */
        Kill_prompt_and_message();  /* BLANK MSG AND PROMPT LINES */

    count = 0;

    /*    FOR EVERY NEW LINE, SCROLL AND WRITE NEW LINE. */

    for (i = first_text_line; i <= last_text_line; i++) {
        if (i < first_of_new) {
            have[i] = have[i + num_line];
            line_size[i] = line_size[i + num_line];
        } else {
            if (config != SIIIE) {
                Put_scroll(1);
                have[i] = next_start;
                next_start = Print_text_line(next_start);
                line_size[i] = line_size[last_text_line];
            } else {
                i = 254;
            }

        }
    }

    if (config == SIIIE) {
        Put_scroll(message_line - first_of_new);
        for (i = first_of_new; i <= last_text_line; i++) {
            have[i] = next_start;
            Put_start_row((byte)i);
            isa_text_line = _TRUE;
            next_start = Print_text_line(next_start);
        }
    }

    have[message_line] = next_start;

    if (config != SIV && !window_present)
        Print_last_prompt_and_msg(_TRUE); /* only small prompt */

    Position_in_text();
    return _TRUE;
} /* scroll */



/*
    REVERSE_SCROLL                BACKUP THE DISPLAY INDICATED NUMBER OF LINES
*/
static boolean Reverse_scroll(byte num_line) {

    int j;
    word count;
    pointer start;
    boolean clean_scroll;

    clean_scroll = (output_codes[delete_out_code].code[0] != 0 &&
                    output_codes[reverse_scroll_out_code].code[0] != 0);

    /* IF CANNOT REVERSE SCROLL (SOB!) CAN ONLY RE_VIEW    */
    if (output_codes[reverse_scroll_out_code].code[0] == 0)
        return _FALSE;

    if (window_present && !clean_scroll)
        return _FALSE;

    /* CHECK TO MAKE SURE REVERSE SCROLLING WILL PUT US WHERE WE WANT TO BE -
       IF NOT WE SHOULD RE-VIEW (CAN HAPPEN ON TOO LONG LINES) */

    start = Backup_line(num_line, have[first_text_line], _FALSE);
    if (start > oa.high_s)
        return _FALSE;

    if (config != SIV && !window_present)
        Kill_prompt_and_message();  /* BLANK MSG AND PROMPT LINES */

    /* FOR EVERY NEW LINE, BACKUP THE DISPLAY AND WRITE THE NEW LINE */

    have[message_line] = have[message_line - num_line];
    count = 0;
    j = last_text_line;
    while (j != first_text_line - 1) {                                /* LAST LINE IS ZERO */
        if (j >= num_line + first_text_line) {
            start = have[j] = have[j - num_line];
            line_size[j] = line_size[j - num_line];
        } else {
            count++;
            Put_reverse_scroll();
            line_size[j] = 0;
            if (config != SIIIE) {
                Print_text_line(have[j] = Backup_line(1, have[j + 1], _FALSE));
                line_size[j] = line_size[0];
            }
        }
        j--;
    }

    if (config == SIIIE) {
        start = Backup_line(count, start, _FALSE);

        count--;
        for (j = 0; j <= count; j++) {
            Put_start_row(j + first_text_line);
            isa_text_line = _TRUE;
            have[j + first_text_line] = start;
            start = Print_text_line(start);
        }
    }


    /* RESTORE MSG AND PROMPT LINES (only small prompt)*/
    if (config != SIV && !window_present) {
        //       Kill_prompt_and_message();      // re-clear due to bug in MS Delete Line
        Print_last_prompt_and_msg(_TRUE);
    }

    Position_in_text();
    return _TRUE;

} /* reverse_scroll */




/*
    RE_VIEW                        CALLED BY MAIN LOOP TO REBUILD THE DISPLAY
*/
void Re_view() {

    wbyte i;
    pointer start;
    byte tmp_b;


    start = 0;
    if (have[first_text_line] > 0) {
        /* HAVE SOMETHING ON SCREEN IF HAVE NOTHING, START OVER */

        if (oa.high_s < have[first_text_line]) {
            /* OFF THE TOP OF THE SCREEN */
            i = Count_lines(oa.high_s, have[first_text_line]);
            /* SEE IF IN RANGE FOR SCROLL. DONT BOTHER IF IN A MACRO */
            if (i > 0 && i < message_line - first_text_line
                && (In_macro_exec() == _FALSE || dont_stop_macro_output)) {

                /* if we can scroll, we are done */

                if (Reverse_scroll((byte)i))
                    return;
            }
        } else if (have[message_line] <= oa.high_s) {
            /* OFF BOTTOM OF SCREEN */
            if (have[message_line] >= oa.low_e) i = 1;
            else i = Count_lines(have[message_line], oa.low_e) + 1;
            /* DO NOT SCROLL IF IN MACRO */
            if (i < prompt_line - first_text_line && (In_macro_exec() == _FALSE
                                                      || dont_stop_macro_output)) {
                if (saved_line == _TRUE) {
                    /* first re_write what we have
                       on screen, then do the scrolling */
                    Re_write(have[first_text_line]);
                }
                if (Scroll((byte)i))
                    return;
            }
        } else {
            start = have[first_text_line];  /* STILL WITHIN DISPLAY */
        }

    } else {             /* FOR RE_VIEW - FLAG NOTHING ON SCREEN */
        memset(&have[first_text_line], 0, sizeof(have[0]) * (message_line - first_text_line));
    }

    if (start == 0) {

        Working();

        first_at_sign_is_here = _FALSE;
        /* this is a b_cmnd's variable that prevents redundant writing.
           when the screen is reprinted the first at-sign disappears */

           /* IF IN MACRO AND HAVE FALLEN OFF OF THE SCREEN, SUPPRESS
           SUBSEQUENT OUTPUT UNTIL ALL MACROS ARE DONE */

        if (In_macro_exec() && dont_stop_macro_output == _FALSE) {
            Co_flush();
            macro_suppress = _TRUE;
        }
        tmp_b = center_line;
        if (window_present)
            tmp_b = ((last_text_line - first_text_line) * center_line) / Max(last_text_line, window.last_text_line);
        start = Backup_line(tmp_b, oa.low_e, _FALSE);
    }

    Re_write(start);                    /* ACTUALLY RE-WRITE SCREEN */
    Position_in_text();

} /* re_view */
