// #SMALL
// #title('TAGS               MANIPULATE THE TAGS')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

/*
    THIS MODULE CONTAINS THE ROUTINES THAT SET AND USE THE TAGS
*/
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"



#define wk1	1
#define wk2	2



/*
    DELETE_BLOCKS                        DELETE THE INDICATED NUMBER OF BLOCKS
                                        FROM EITHER WK1 OR WK2. MAKE ANY
                                        DELETED TAGS POINT TO CURRENT LOCATTAGSN.
*/
static void Delete_blocks(byte wk1_or_wk2, word ndelete) {
    word i;

    if (ndelete == 0)
        return;
    for (i = 1; i <= ndelete; i++) {                 /* BACK UP THE DESIGNATED WORK FILE */
        if (wk1_or_wk2 == wk1)
            Backup_temp(oa.wk1_conn);
        else
            Backup_temp(oa.wk2_conn);
    }

    /*    FIX TAGS FOR CASE WHERE WK1 BLOCKS DELETED */

    if (wk1_or_wk2 == wk1) {
        for (i = 1; i <= num_tag; i++) {
            if (oa.tblock[i] >= oa.wk1_blocks - ndelete)
                if (oa.tblock[i] < oa.wk1_blocks) {
                    oa.tblock[i] = oa.wk1_blocks - ndelete;
                    oa.toff[i] = oa.high_s;
                }
                else oa.tblock[i] = oa.tblock[i] - ndelete;
        }
        oa.wk1_blocks = oa.wk1_blocks - ndelete;
    }

    /*    FIX TAGS FOR THE CASE WHERE WK2 BLOCKS WERE DELETED */

    else {
        for (i = 1; i <= num_tag; i++) {
            if (oa.tblock[i] > oa.wk1_blocks)
                if (oa.tblock[i] <= oa.wk1_blocks + ndelete) {
                    oa.tblock[i] = oa.wk1_blocks;
                    oa.toff[i] = oa.high_s;
                }
                else oa.tblock[i] = oa.tblock[i] - ndelete;
        }
        oa.wk2_blocks = oa.wk2_blocks - ndelete;
    }
} /* delete_blocks */





/**********    THE TAG ROUTINES FOLLOW   ***********/



/* SET THE TAG TO THE SPECIFIED LOCATION IN MEMORY. */

void Set_tag(byte tagn, pointer in_mem) {

    oa.tblock[tagn] = oa.wk1_blocks;
    oa.toff[tagn] = in_mem;
} /* set_tag */




void Set_tagi_at_lowe() {

    Set_tag(ed_tagi, oa.low_e);
} /* set_tagi_at_lowe */




void Jump_tag(byte tagn) {

    while (oa.tblock[tagn] != oa.wk1_blocks) {
        if (oa.tblock[tagn] < oa.wk1_blocks)
            Backup_file();
        else
            Forward_file();
    }
    cursor = oa.toff[tagn];
    Re_window();
} /* jump_tag */




void Jump_tagi() {

    Jump_tag(ed_tagi);
} /* jump_tagi */



/************************************************************
CALLED AFTER  ANY DELETE.  IF ANY  TAGS POINT TO SPACE WITHIN
THE gap, THEY ARE INCREMENTED TO POINT TO THE NEXT CHARACTER.
************************************************************/
void Clean_tags() {
    wbyte i;
    pointer saver;

    saver = oa.toff[ed_tagw];

    for (i = 1; i <= num_tag; i++) {
        if (oa.tblock[i] == oa.wk1_blocks && oa.toff[i] >= oa.low_e && oa.toff[i] < oa.high_s)
            oa.toff[i] = oa.high_s;
    }

    /* PRESEVE the setting of ed_tagw if we have called clean tags
       while not in the same file that that tag is reserved for */
    if (in_other != w_in_other)
        oa.toff[ed_tagw] = saver;

} /* clean_tags */




/* DELETE TEXT BETWEEN CURRENT LOCATION AND THE DESIGNATED TAG. */

void Delete_to_tag(byte tagn) {
    word dest_block;
    pointer dest_off;
    byte i;

    dest_block = oa.tblock[tagn];
    if (dest_block < oa.wk1_blocks) {        /* THE TAG IS IN WK1 FILE */
        oa.low_e = oa.low_s;                        /* DELETE EVERYTHING BELOW WINDOW*/
        Clean_tags();
        Delete_blocks(wk1, oa.wk1_blocks - dest_block - 1);
        i = Backup_file();
        oa.low_e = oa.toff[tagn];                /* DELETE PART OF LAST BLOCK */
        Clean_tags();
        V_cmnd();                        /* BUILD SCREEN FROM SCRATCH */
        return;
    }

    if (dest_block > oa.wk1_blocks) {    /* TAG IS IN WK2 FILE */
        V_cmnd();                        /* BUILD SCREEN FROM SCRATCH */
        oa.high_s = oa.high_e;                /* DELETE ALL AFTER WINDOW */
        Clean_tags();
        Delete_blocks(wk2, dest_block - oa.wk1_blocks - 1);
        i = Forward_file();                    /* READ TAG BLOCK INTO MEMORY */
    }

    /*    NOW HAVE THE TAG IN MEMORY */

    dest_off = oa.toff[tagn];
    if (dest_off <= oa.low_e) {            /* AT END OF DELETE RANGE */
        if (have[first_text_line] <= dest_off) {
            i = row;                            /* SEE IF STILL ON THE SCREEN */
            while ((have[i] > dest_off)) {        /* FIND THE STARTING ROW */
                i--;
            }
            Save_line(i);
        }
        else {
            V_cmnd();
        }
        oa.low_e = dest_off;
        Clean_tags();
    }
    else if (dest_off > oa.high_s) {        /* TAG IS PAST CURSOR */
        if (have[first_text_line] != 0) Save_line(row);/* WILL DELETE END OF ROW */
        oa.high_s = dest_off;
        Clean_tags();
        /* MIGHT HAVE TO LIE ABOUT DISPLAY TO AVOID SCROLLING */
        Dont_scroll();
    }
} /* delete_to_tag */

