// #COMPACT
// #subtitle('===> VIRTUAL FILE MANAGER <===')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#ifdef MSDOS
#include <io.h>
#endif
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"


// TODO: fix selector usage

static void Unlink_from_start(byte file_num);

/** THIS MODULE IMPLEMENTS A SIMPLE SYSTEM FOR VIRTUAL FILES.
    BLOCKS ARE BUFFERED IN MEMORY AS LONG AS POSSIBLE RATHER THAN
    GOING IMMEDIATELY TO DISK.
**/


/** UDI EXTERNALS **/



#define dq_read_mode	1
#define dq_write_mode	2
#define dq_update_mode	3


#define relative_back_mode	1
#define abs_mode	2
#define relative_forward_mode	3
#define end_minus_mode	4



/** TYPES **/

#pragma pack(push , 1)

typedef struct _blk {
    struct _blk *next;
    struct _blk *prev;
    struct _blk *next_lru;
    struct _blk *prev_lru;
    byte file_num;
    byte data_bytes[0];
} block_t;

/* The block's header size (4*4+1). */
#define header_size sizeof(block_t)

typedef struct {
    byte status;
    byte mode;
    connection conn;
    word total_blocks;
    word cur_block_num;
    word blocks_on_disk;
    block_t *first_block_ptr;
    block_t *last_block_ptr;
    block_t *cur_block_ptr;
} vf_struc_t;

char *template = "_workXXXXXX";
/*
   The fields of the structure above:

                        status BYTE,             |* physical file is detached, attached or opened *|
                        mode BYTE,               |* regular, view_only_wk1 or view_only_wk2. *|
                        conn WORD,               |* Physical file connection. *|
                        total_blocks WORD,       |* Number of blocks in file.*|
                        cur_block_num WORD,      |* Current block number. *|
                        blocks_on_disk WORD,     |* Number of blocks on disk.*|
                        first_block_ptr POINTER, |* first buffer in memory *|
                        last_block_ptr POINTER,  |* last buffer in memory *|
                        cur_block_ptr POINTER)   |* current block address *|
*/

#pragma  pack(pop)


/** STATUS TYPES **/

#define detached_stat	0
#define attached_stat	1
#define opened_stat	2




/** LOCALS **/

vf_struc_t virtual_files[4] = {
                detached_stat, keep_file, NULL, 0, 0, 0, NULL, NULL, NULL,
                detached_stat, keep_file, NULL, 0, 0, 0, NULL, NULL, NULL,
                detached_stat, keep_file, NULL, 0, 0, 0, NULL, NULL, NULL,
                detached_stat, keep_file, NULL, 0, 0, 0, NULL, NULL, NULL };




static void Check() {
    if (excep != 0) {
        file_num = work_file;
        Echeck();
        V_cmnd();
        cursor = oa.low_s;
        Re_window();
        Forward_line(center_line + 2);
        longjmp(aedit_entry, 1);
    }
} /* check */






/********* Block allocation system. ************/


 /* Pointers for the LRU mechanism. */
block_t *oldest_block = NULL;
block_t *newest_block = NULL;




block_t *free_list = NULL;


/* mechanism to limit the number of blocks allocated by dq$allocate. */

dword use_mem_blocks = 0;
word max_tmpman_mem = 0xffff; /* in Kbyte */
  /* CAN get a value from the macro file.*/



static void Free_block(block_t *blk_p) {
    /**********************************************************************
       Free an unused block. Currently , dq$free is not called, but a
       free-blocks list is maintained.
    **********************************************************************/

#ifdef MSDOS
    blk_p->next = free_list;
    free_list = blk_p;
#else
    free(blk_p);
    if (errno)
        excep = E_MEM;
    use_mem_blocks--;
#endif
} /* free_block */



static block_t *Get_block() {
    /**********************************************************************
       Try to allocate a block. If the free-list is not empty, get the
       block from there. Else try to (dq$)allocate a block; If can't,
       try to spill to disk the least recently used block, and get its
       place. If no blocks in free memory, return NIL.
    **********************************************************************/

    block_t *blk_p;
    block_t *newblk_p;

    if (free_list) {
        /* There are free allocated blocks. */
        blk_p = free_list;
        free_list = blk_p->next;
        return blk_p;
    }

    if ((use_mem_blocks + 1) * (block_size + header_size) / 1024 <= max_tmpman_mem) {
        /* Try to allocate a block. */
        newblk_p = (block_t *)malloc(block_size + header_size);
        if (newblk_p) {
            use_mem_blocks++;
            return newblk_p;
        }
        excep = E_MEM;
    }

    /* Write the oldest block to file, and return its buffer. */
    if ((blk_p = oldest_block))
        Unlink_from_start(blk_p->file_num);
    return blk_p;
    /* Returns NIL if there is no block in memory. */
} /* get_block */





/********* Least Recently Used  linked list. ************/


static void Link_to_lru(block_t *blk_p) {
    /**********************************************************************
       Link a block at the end of the LRU list.
    **********************************************************************/
    block_t *tmpblk_p;


    blk_p->next_lru = NULL;
    if (oldest_block == NULL) { /* and newest_block too */
        /** FIRST ITEM ENTERED **/
        blk_p->prev_lru = NULL;
        oldest_block = newest_block = blk_p;
    }
    else {
        blk_p->prev_lru = tmpblk_p = newest_block;
        newest_block = tmpblk_p->next_lru = blk_p;
    }
} /* link_to_lru */



static void Unlink_from_lru(block_t *blk_p) {
    /**********************************************************************
       Unlink a block from the LRU list by connecting its neigbours.
    **********************************************************************/

    block_t *tmpblk_p;

    if (newest_block == blk_p)
        newest_block = blk_p->prev_lru;
    if (oldest_block == blk_p)
        oldest_block = blk_p->next_lru;
    tmpblk_p = blk_p->next_lru;
    if (tmpblk_p)
        tmpblk_p->prev_lru = blk_p->prev_lru;
    tmpblk_p = blk_p->prev_lru;
    if (tmpblk_p)
        tmpblk_p->next_lru = blk_p->next_lru;
} /* unlink_from_lru */


/****************************************************/


#if 0
nestedProc boolean Is_open(byte i, byte *file_num) {

    if (virtual_files[i].status == opened_stat) {
        *file_num = i;
        return TRUE;
    }
    return FALSE;
} /* is_open */

boolean Handle_aftn_full() {
    /**********************************************************************
      Called when an attempt is done to open more than six files at the
      same time. Decides which work file to close temporarily according
      to a simple precedence. Closing this file allows openning the new one.
      The temporary closing requires that open_if_necessary will be called
      before each operation with physical workfiles.
    **********************************************************************/

    byte file_num;
    vf_struc_t *vf_base;




    if (excep != E_SIX && excep != E_MEM)
        return FALSE; /* false - don't retry to open */
    if (!in_other) {
        /* in main; first try in other. */
        if (!Is_open(3, &file_num) /* wk2 of other */ &&
            !Is_open(2, &file_num) /* wk1 of other */ &&
            !Is_open(1, &file_num) /* wk2 of main  */ &&
            !Is_open(0, &file_num))   /* wk1 of main  */
            return FALSE;
    }
    else {
        /* in other; first try in main. */
        if (!Is_open(1, &file_num) /* wk2 of main  */ &&
            !Is_open(0, &file_num) /* wk1 of main  */ &&
            !Is_open(3, &file_num) /* wk2 of other */ &&
            !Is_open(2, &file_num))   /* wk1 of other */
            return FALSE;
    }
    vf_base = &virtual_files[file_num];
    dq_close(vf_base->conn, &excep);
    Echeck();
    vf_base->status = attached_stat;
    return TRUE;

} /* handle_aftn_full */

#endif


static void Open_if_necessary(byte file_num) {
    /**********************************************************************
       If the file is not already attaced - create it.
       Then, if the file is not already opened - open it.
    **********************************************************************/
    vf_struc_t *vf_base;

    vf_base = &virtual_files[file_num];
    if (vf_base->status == detached_stat) {
        if ((vf_base->conn = tmpfile()) == NULL) {
            excep = errno;
            Check();
        }
        vf_base->status = opened_stat;
    }
} /* open_if_necessary */



static void Link_to_end(byte file_num, block_t *blk_p) {
    /**********************************************************************
       Link a new block onto the end of the block chain for that file.
    **********************************************************************/

    block_t * tblk_p;
    vf_struc_t *vf_base;

    vf_base = &virtual_files[file_num];
    blk_p->file_num = file_num;
    blk_p->next = NULL;
    if (vf_base->first_block_ptr == NULL) {    /* add on first buffer ever */
        /** FIRST ITEM ENTERED **/
        blk_p->prev = NULL;
        vf_base->cur_block_ptr = vf_base->last_block_ptr = vf_base->first_block_ptr = blk_p;
    }
    else {            /* add on another buffer */
        blk_p->prev = tblk_p = vf_base->last_block_ptr;
        vf_base->cur_block_ptr = vf_base->last_block_ptr = tblk_p->next = blk_p;
    }
    vf_base->cur_block_num++;
    vf_base->total_blocks++;
    Link_to_lru(blk_p);
} /* link_to_end */





static block_t *Unlink_from_end(byte file_num) {
    /**********************************************************************
       Unlink a block from the end of the block chain.
    **********************************************************************/
    block_t *blk_p, *tblk_p;
    vf_struc_t *vf_base;

    vf_base = &virtual_files[file_num];
    if ((blk_p = vf_base->last_block_ptr)) {
        tblk_p = vf_base->cur_block_ptr = vf_base->last_block_ptr = blk_p->prev;
        if (tblk_p) {
            tblk_p->next = NULL;
        }
        else {
            vf_base->first_block_ptr = NULL;
        }
        vf_base->cur_block_num = vf_base->cur_block_num - 1;
        vf_base->total_blocks = vf_base->total_blocks - 1;
        Unlink_from_lru(blk_p);
    }
    return blk_p;
} /* unlink_from_end */





static void Unlink_from_start(byte file_num) {
    /**********************************************************************
       Spill to disk the first block in the file chain, and remove it
       from the chain.
    **********************************************************************/

    block_t *blk_p, *tblk_p;
    vf_struc_t *vf_base;
    dword dtemp;

    vf_base = &virtual_files[file_num];
    blk_p = vf_base->first_block_ptr;
    if (vf_base->mode == keep_file) {
        Open_if_necessary(file_num);
        dtemp = vf_base->blocks_on_disk * block_size;
        dq_seek(vf_base->conn, abs_mode, dtemp, &excep);
        Check();
        fwrite(blk_p->data_bytes, 1, block_size, vf_base->conn);
        excep = errno;
        Check();
    }
    vf_base->blocks_on_disk++;

    tblk_p = vf_base->first_block_ptr = blk_p->next;
    if (tblk_p) {
        tblk_p->prev = NULL;
    }
    else {
        vf_base->last_block_ptr = NULL;
    }
    if (vf_base->cur_block_ptr == blk_p)
        vf_base->cur_block_ptr = tblk_p;
    Unlink_from_lru(blk_p);
} /* unlink_from_start */





void Reinit_temp(byte file_num) {
    /**********************************************************************
       Frees all the blocks associated with that file, and resets
       associated variables. If the mode is keep_file, the physical file
       is rewineded.
    **********************************************************************/
    block_t *blk_p, *next_ptr;
    vf_struc_t *vf_base;

    Working();

    vf_base = &virtual_files[file_num];

    if ((vf_base->mode == keep_file)) {
        /* seek to beginning of workfile if open */
        if (vf_base->status != detached_stat) {
            excep = fclose(vf_base->conn) ? errno : 0;
            vf_base->conn = NULL;
            vf_base->status = detached_stat;
        }
    }
    else {
        vf_base->status = detached_stat;
        /* The caller does the physical dq$detach. */
    }

    /** FREE ALL ASSOCIATED BUFFERS **/
    blk_p = vf_base->first_block_ptr;
    while (blk_p) {
        next_ptr = blk_p->next;
        Unlink_from_lru(blk_p);
        Free_block(blk_p);
        blk_p = next_ptr;
    }
    vf_base->first_block_ptr = NULL;
    vf_base->last_block_ptr = NULL;
    vf_base->cur_block_ptr = NULL;
    vf_base->blocks_on_disk = 0;
    vf_base->cur_block_num = 0;
    vf_base->total_blocks = 0;
    vf_base->mode = keep_file;

} /* reinit_temp */





void Set_temp_viewonly(byte file_num, byte mode, connection conn) {
    /**********************************************************************
    **********************************************************************/

    vf_struc_t *vf_base;

    vf_base = &virtual_files[file_num];

    vf_base->mode = mode;
    vf_base->status = opened_stat;
    vf_base->conn = conn;

} /* set_temp_viewonly */





void Read_temp(byte file_num, pointer buf_addr) {
    /**********************************************************************
       Read the current block of virtual_files(file_num) and moves it to
       buf_addr. The read blocks are not removed from the file.
       The current block's successor becomes the current block.
       No need to OPEN_IF_NECESSARY, because rewind does the job.
    **********************************************************************/
    word actual;
    block_t *blk_p;
    vf_struc_t *vf_base;

    vf_base = &virtual_files[file_num];
    if (vf_base->cur_block_num <= vf_base->blocks_on_disk) {
        actual = (byte)fread(buf_addr, 1, block_size, vf_base->conn);
        excep = ferror(vf_base->conn) ? errno : 0;
        Check();
        vf_base->cur_block_num = vf_base->cur_block_num + 1;
        return;
    }
    /** CHECK IF BUFFERS AVAILABLE **/
    blk_p = vf_base->cur_block_ptr;
    if (blk_p) {
        vf_base->cur_block_ptr = blk_p->next;
        vf_base->cur_block_num = vf_base->cur_block_num + 1;
        memcpy(buf_addr, blk_p->data_bytes, block_size);        // block size assumed to be even
    }
} /* read_temp */




logical Read_previous_block(byte file_num, pointer buf_addr, logical keep) {
    /**********************************************************************
       Read the current block of virtual_files(file_num) and moves it to
       buf_addr. first reads are taken from memory, then it starts to
       read from the disk file if it was necessary to have spilled it to
       disk. If keep is false, the read blocks are removed from the file.
       The current block's predecessor becomes the current block.
    **********************************************************************/
    word actual;
    block_t *blk_p;
    vf_struc_t *vf_base;

    Working();

    vf_base = &virtual_files[file_num];

    /* check for beginning of file */
    if (vf_base->cur_block_num == 0) return _FALSE;
    if (vf_base->cur_block_num > vf_base->blocks_on_disk) {
        /** First check if buffers available **/
        if (keep) {
            blk_p = vf_base->cur_block_ptr;
            if (blk_p)
                vf_base->cur_block_ptr = blk_p->prev;
            vf_base->cur_block_num = vf_base->cur_block_num - 1;
        }
        else {
            blk_p = Unlink_from_end(file_num);
        }
        if (blk_p == NULL)
            return _FALSE;
        memcpy(buf_addr, blk_p->data_bytes, block_size);        // block size assumed to be even
        if (!keep)
            Free_block(blk_p);
        return _TRUE;
    }
    else {
        /* Try to read from the disk file. */
        dword dtemp;
        Open_if_necessary(file_num);
        /* cur_block_num can't be zero because of check above */
        if (vf_base->mode == view_only_wk2) {
            dtemp = block_size * (virtual_files[file_num - 1].cur_block_num - 1);
            /* file_num-1 is the number of the wk1 that "belongs"
               to this wk2 */
            dtemp = dtemp + block_size + Size_of_text_in_memory();
        }
        else {
            dtemp = block_size * (vf_base->cur_block_num - 1);
        }
        dq_seek(vf_base->conn, abs_mode, dtemp, &excep);
        Check();
        actual = (byte)fread(buf_addr, 1, block_size, vf_base->conn);
        excep = ferror(vf_base->conn) ? errno : 0;
        Check();
        /*
          are these two lines redundant now ?
                CALL dq$seek(vf_base->conn, abs$mode, HIGH(dtemp), LOW(dtemp), @excep);
                CALL check;
        */
        if (!keep) {
            vf_base->blocks_on_disk--;
            vf_base->total_blocks--;
        }
        vf_base->cur_block_num--;
        return _TRUE;
    }
} /* read_previous_block */




void Backup_temp(byte file_num) {
    /**********************************************************************
       Remove the last block of the file.
    **********************************************************************/
    block_t *blk_p;
    vf_struc_t *vf_base;

    Working();

    vf_base = &virtual_files[file_num];

    /* Check for beginning of file. */
    if (vf_base->cur_block_num == 0)
        return;
    if (vf_base->cur_block_num > vf_base->blocks_on_disk) {
        blk_p = Unlink_from_end(file_num);
        Free_block(blk_p);
    }
    else {
        /* No memory buffers to backup through; backup on disk. */
        if (vf_base->mode == keep_file) {
            dword dtemp;
            Open_if_necessary(file_num);
            /* Cur_block_num can't be zero because of check above. */
            dtemp = block_size * (vf_base->cur_block_num - 1);
            dq_seek(vf_base->conn, abs_mode, dtemp, &excep);
            Check();
        }
        vf_base->total_blocks--;
        vf_base->blocks_on_disk--;
        vf_base->cur_block_num--;
    }
} /* backup_temp */




void Skip_to_end(byte file_num) {
    /**********************************************************************
       Set the pointers such that we are at the end of the file.
       No need to OPEN_IF_NECESSARY, because read_previous_block does the job.
    **********************************************************************/

    vf_struc_t *vf_base;

    Working();

    vf_base = &virtual_files[file_num];
    if (vf_base->total_blocks == 0)
        return;
    /* seek to end of workfile if open */
    if (vf_base->blocks_on_disk > 0) {
        dword dtemp;
        dtemp = vf_base->blocks_on_disk * block_size;
        dq_seek(vf_base->conn, abs_mode, dtemp, &excep);
        Check();
    }
    vf_base->cur_block_ptr = vf_base->last_block_ptr;
    vf_base->cur_block_num = vf_base->total_blocks;
} /* skip_to_end */





void Rewind_temp(byte file_num) {
    /**********************************************************************
       Set the pointers such that we are at the beginning of the file.
    **********************************************************************/

    vf_struc_t *vf_base;

    Working();

    vf_base = &virtual_files[file_num];
    if (vf_base->total_blocks == 0)
        return;
    /* seek to beginning of workfile if open */
    if (vf_base->blocks_on_disk > 0) {
        Open_if_necessary(file_num);
        dq_seek(vf_base->conn, abs_mode, 0L, &excep);
        Check();
    }
    vf_base->cur_block_ptr = vf_base->first_block_ptr;
    vf_base->cur_block_num = 1;
} /* rewind_temp */





void Restore_temp_state(byte file_num) {
    /**********************************************************************
      The  PROC copy_wk1 causes vf_base->cur_block_ptr to be NIL.  In this
      situation  it is the caller's responsibility to recover the current
      setting, using this PROC.
    **********************************************************************/

    vf_struc_t *vf_base;

    vf_base = &virtual_files[file_num];
    vf_base->cur_block_ptr = vf_base->last_block_ptr;
    vf_base->cur_block_num = vf_base->total_blocks;
} /* restore_temp_state */





void Write_temp(byte file_num, pointer buf_addr) {
    /**********************************************************************
       Write to the given file the data at buf_addr. Try to put in free
       memory first, otherwise, spill the oldest written data to disk,
       keeping the newer data in memory if possible. If there are no
       blocks in memory at all, spill the new block to disk.
    **********************************************************************/
    block_t *blk_p;
    vf_struc_t *vf_base;
    dword dtemp;

    Working();

    vf_base = &virtual_files[file_num];
    blk_p = Get_block();
    if (blk_p) {
        memcpy(blk_p->data_bytes, buf_addr, block_size);        // block size assumed to be even
        Link_to_end(file_num, blk_p);
    }
    else {
        if (vf_base->mode == keep_file) {
            Open_if_necessary(file_num);
            dtemp = vf_base->blocks_on_disk * block_size;
            dq_seek(vf_base->conn, abs_mode, dtemp, &excep);
            Check();
            fwrite(buf_addr, 1, block_size, vf_base->conn);
            excep = errno;
            Check();
        }
        vf_base->blocks_on_disk++;
        vf_base->total_blocks++;
        vf_base->cur_block_num++;
    }
} /* write_temp */





void Tmpman_init() {


    byte i;

    for (i = 0; i <= last(virtual_files); i++) {
        virtual_files[i].first_block_ptr = NULL;
        virtual_files[i].last_block_ptr = NULL;
        virtual_files[i].cur_block_ptr = NULL;
    }
    oldest_block = newest_block = free_list = NULL;

} /* tmpman_init */


