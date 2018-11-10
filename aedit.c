// #small
// #title('AEDIT                     MAIN ROUTINE FOR ALTER EDITOR')
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

/*  Modified 4/20/89 by Douglas Jackson for iRMX II.4.  Changed
 *  to allow commands which use rq_c_send_?o_response to work properly
 *  with the !command.  (Put console-in in line-edit mode before
 *  calling System.)  Find 'dhj'.
 */
#include <setjmp.h>     // for jmp_buf
#include <stdlib.h>     // for system
#include <stdbool.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"
#include <signal.h>


 /**************************************************************************/
 /*                            Last_dispatch                               */
 /*                                                                        */
 /*                                                                        */
 /**************************************************************************/
static void Last_dispatch() {
    if (last_main_cmd == 'H')
        H_cmnd();
    else if (last_main_cmd == 'P')
        P_cmnd();
    else
        Fr_cmnd();
} /* Last_dispatch */


static void V1_cmnd() {
    Put_home();
    /* erase current window.  set current prompt and message to null */
    Put_clear_all_text();
    Init_prompt_and_message();
    Print_banner();
    V_cmnd();               /* FORCE RE-WRITE OF SCREEN */
    Re_view();
} /* V1_cmnd */


byte s_system[string_len_plus_2] = { 0 };

static void System_call() {
    bool save_input_expected;
    byte save_prompt_line;

#ifdef MSDOS
    if (!dos_system) {
        Error("\x1c" "System command not supported");
        return;
    }
#endif
    if (Input_line("\x10System command: ", s_system) == CONTROLC)
        return;
    Move_name(input_buffer, s_system);
    force_writing = _TRUE;
    Reset_scroll_region();
    Put_erase_entire_screen();
    Put_goto(0, 0);
    Co_flush();
    V_cmnd();

#ifdef MSDOS
    s_system[s_system[0] + 1] = 0;  /* Convert to null terminated string */
    if (system(&s_system[1]) != 0)  /* for dos */
        Error("\x20" "unable to execute System command");
    excep = signal(SIGINT, Cc_trap) == SIG_ERR ? errno : 0; /* MUST TRAP CONTROL C AGAIN */
    Echeck();
#else // Unix
    word err;
    s_system[s_system[0] + 1] = 0;  /* convert to null terminated string */
    set_ci_mode(2);     // normal
    Restore_quit_signal();
    err = system(&s_system[1]);
    Ignore_quit_signal();
    set_ci_mode(poll_mode ? 3 : 1);
    if (err && err < 256)
        Error("\x20" "unable to execute system command"));
#endif
        Put_scroll_region(first_text_line, last_text_line);
        save_input_expected = input_expected_flag;
        Set_input_expected('-'); /* suppress the indicator */
        save_prompt_line = prompt_line;
        prompt_line = (byte)Max(prompt_line, window.prompt_line);
        Hit_space();
        prompt_line = save_prompt_line;
        if (save_input_expected)
            Set_input_expected('!'); /* set the previous indicator */
        Put_erase_entire_screen();
        if (window_present) {
            W_cmnd();
            V_cmnd();
            Re_view();
            Init_prompt_and_message();
            Print_last_prompt_and_msg(_FALSE);
            W_cmnd();
        }
        force_writing = _FALSE;

} /* PROC System_call() */

jmp_buf aedit_entry;




/* MAIN COMMAND DISPATCH TABLE. ALL COMMANDS EXCEPT MOVE COMMANDS ARE HERE. */

dispatch_t cmnd_dispatch[32] = {
    delete_code, Delete_cmnd,
    delete_line_code, Delete_line_cmnd,
    delete_left_code, Delete_left_cmnd,
    delete_right_code, Delete_right_cmnd,
    undo_code, Undo_cmnd,
    CR, Cr_cmnd,
    rubout_code, Rubout_cmnd,
    TAB, Tab_cmnd,
    '-', Fr_cmnd,
    '?', Fr_cmnd,
    'A', Last_dispatch,
    'B', B_cmnd,
    'C', C_cmnd,
    'D', B_cmnd,
    'E', E_cmnd,
    'F', Fr_cmnd,
    'G', G_cmnd,
    'H', H_cmnd,
    'I', I_cmnd,
    'J', J_cmnd,
    'K', K_cmnd,
    'M', M_cmnd,
    'O', O_cmnd,
    'P', P_cmnd,
    'Q', Q_cmnd,
    'R', Fr_cmnd,
    'S', S_cmnd,
    'T', T_cmnd,
    'V', V1_cmnd,
    'W', W_cmnd,
    'X', X_cmnd,
    0, NULL };

byte forbidden_in_viewonly[] = {
    delete_code, delete_line_code, delete_left_code,
    delete_right_code, undo_code, rubout_code,
    '?', 'D', 'G', 'I', 'P', 'R', 'X' };

pointer prompts[4] = {
   "\x4f" RVID "Again     Block     Calc      Delete   "
          RVID "Execute   Find      -Find     --more--",
   "\x4f" RVID "Get       Hex       Insert    Jump     "
          RVID "Kill_wnd  Macro     Other     --more--",
   "\x4f" RVID "Paragraph Quit      Replace   ?replace "
          RVID "Set       Tag       View      --more--",
   "\x4f" RVID "Window    Xchange   !system            "
          RVID "                              --more--"
};

/* THE MAIN ENTRY POINT OF ALTER */
void Aedit() {
    Alter_init();
    /* aedit_entry: */
    setjmp(aedit_entry); /* jumping here after calc errors, or tmpman errors, to clean the stack */
    while (1) {
        /* LOOPS FOREVER && DISPATCHES ON ANY CHARACTER TYPED */
        if (Have_controlc())
            cc_flag = _FALSE;

        Re_view();                    /* DISPLAY THE SCREEN    */

        quit_state = 0xff;
        command = Input_command(prompts[prompt_number]);
        quit_state = 0;

        if (command == 'A') {
            /*IF LAST COMMAND WAS A FIND TYPE, LEAVE FR_CMND WITH COMMAND=A */
            if (Find_index(last_main_cmd, "\x6" "FR?-HP") < 6) {
                if (oa.file_disposition == view_only && (last_main_cmd == 'R' || last_main_cmd == '?' || last_main_cmd == 'P'))
                    command = 0;
            }
            else
                /* MAKE COMMAND SAME AS LAST COMMAND    */
                command = last_main_cmd;    /* HANDLE Again HERE */
        }
        else
            last_main_cmd = command;

        if (command == '!') {
            System_call();
            longjmp(aedit_entry, 0);
        }

        if (oa.file_disposition == view_only)
            if (findb(forbidden_in_viewonly, command, sizeof(forbidden_in_viewonly)) != 0xffff)
                command = 0; /* Illegal command */

        if (Move_cmnd(command) == _FALSE) { /* DO A MOVE CURSOR COMMAND */
            /* DONT BOTHER WITH MESSAGE OR DISPATCHING IF USER HITS ESCAPE OR CC */
            if (command != CONTROLC && command != esc_code)
                if (Dispatched(command, cmnd_dispatch) == _FALSE)
                    if (command == macro_exec_code)
                        Handle_macro_exec_code();
                    else if (!Single_char_macro(command))
                        Illegal_command();
        }
    }


} /* Aedit */

