#pragma once
#include "type.h"
#include <memory.h>     // for movb etc. replacements

#define nestedProc  static
void toCstr(char *cstr, byte *pstr);
void co_write(byte *buf, word len);
void set_ci_mode(byte mode);
word ci_read(byte *buf);

void Aedit();

/* note needs more work to define paramater types */
/*****     CCTRAP.PLM     *****/
void Cc_trap(int);

/*****     AEDPLM.PLM     *****/
void Position_in_text();
pointer Unfold(pointer line_p, pointer image_p);
pointer Next_line(pointer ptr);
pointer Prior_line(pointer ptr);

/*****     BLOCK.PLM     *****/
void B_cmnd();
void G_cmnd();


/*****     CALC.PLM     *****/
pointer Get_n_var(byte radix, boolean add_minus);
pointer Get_s_var();
void C_cmnd();

/*****     CMNDS.PLM     *****/
byte Dispatched(byte command, dispatch_t *table_p);

static void Reset_column();
void Set_dirty();
void Dont_scroll();
void Test_crlf();
void Insert_a_char(byte ch);
void Insert_crlf();
void Jump_start();
void Cr_cmnd();

void Tab_cmnd();
void Delete_cmnd();
void Delete_line_cmnd();
void Delete_left_cmnd();
void Delete_right_cmnd();
byte Move_cmnd(byte ch);
void Rubout_cmnd();
void Undo_cmnd();
byte Hex_value(byte _char);
static boolean Convert_string_to_hex();
// Convert_string_to_hex(pointer source_p, pointer target_p);
void H_cmnd();
void I_cmnd();
void J_cmnd();
void T_cmnd();
void X_cmnd();
void V_cmnd();


/*****     COMAND.PLM     *****/
void Print_input_line();
byte Cmd_ci();
byte Upper(byte ch);
byte Is_illegal(byte ch);
byte Not_cursor_movement(byte cmd);
void Add_str_special(pointer str_p);
byte Input_line(pointer prompt, pointer prev_string_p);
byte Input_filename(pointer prompt, pointer prev_string_p);
byte Input_yes_no(pointer prompt, byte y_type);
byte Input_yes_no_from_console(pointer prompt, byte y_type, boolean in_replace);
byte Input_command(pointer prompt);
byte Input_fr();
byte Hit_space();
byte Get_hexin(boolean *err_p);

/*****     CONF.PLM     *****/
void Insert_long_config(pointer new_str_p, entry_t *entry_p);
void SIV_setup();
void SIII_setup();
void SIIIE_setup();
void SIIIET_setup();
void PCDOS_setup();
void VT100_setup();
void ANSI_setup();
void Ibm_pc_setup();
void Reset_config();
void Setup_terminal();
void Check_minimal_codes_set();
static void Update_config_table(pointer table_p, word table_len, word entry_size, pointer new_data_p);
void Restore_system_config();
void Close_ioc();

/*****     CONSOL.PLM     *****/

void AeditBeep();
byte Printable(byte ch);
void Text_co(byte ch);
void Print_line(pointer line);
static void Print_message_and_stay(pointer line);
void Print_count_message();
void Print_message(pointer line);
void Error(pointer msg_p);
void Early_error(pointer msg);
void Illegal_command();
void Kill_message();
void Clear_message();
void Clear_count_message();
void Gone_prompt();
void Kill_prompt_and_message();
void Init_prompt_and_message();
void Print_prompt(pointer line);
void Print_prompt_and_repos(pointer line);
void Print_last_prompt_and_msg(boolean small_prompt);
void Rebuild_screen();

/*****    FIND.PLM    *****/
void Fr_cmnd();

/*****     IO.PLM     *****/
void Set_input_expected(byte ch);
void Print_unconditionally_p(pointer string);
void Print_unconditionally(char *string);
void Print_unconditionally_crlf();
void Handle_controlc();
void Co_flush();
void Print_string(pointer string);
void Print_update(byte cur_column, byte len, pointer string, boolean rev_vid);
void Check_for_keys();
void Set_cursor(byte new_val);
byte Ci();
void Echeck();
void Echeck_no_file();
void Openi(byte fnum, byte nbuf);
byte Test_file_existence(byte fnum);
void Openo(byte fnum, byte mode, byte nbuf);
void Open_cico();
void Detach(byte fnum);
void Detach_input();
word Read(byte fnum);
static void Rewind(byte fnum);
static void Expand_window();
void Check_window(word plusplus);
word Macro_file_read();
byte Can_backup_file();
byte Backup_file();
byte Can_forward_file();
byte Forward_file();
void Write(byte nfile, pointer buf, word len);
void Write_block_file(byte do_delete);
void Write_util_file();
void Read_util_file();
void Read_block_file();
byte Have_controlc();
void Working();

/*****     MACROF.PLM     *****/
void Macro_file_error(pointer string);
byte Macro_byte();
static byte Macro_not_blank();
boolean Add_macro_char(byte ch);
// void Mark_param_start();
// void Ignore_param();
static byte Find_macro();
byte Find_index(byte ch, pointer ch_list_p);
void Macro_file_process();
void Stop_macro();
static void Macro_add_level();
logical Single_char_macro(byte ch);
void Exec_init_macro();
void Handle_macro_exec_code();
byte In_macro_exec();
byte Supply_macro_char();
void E_cmnd();
void M_cmnd();

/*****     SCREEN.PLM     *****/
// byte Port_input(word port_num);
void Put_home();
void Put_left();
void Put_erase_screen();
void Put_erase_entire_screen();
void Put_erase_line();
void Put_erase_entire_line();
void Put_normal_video();
void Adjust_for_column_80();
void Put_reverse_video();
void Put_re_col(byte new_col);
void Put_goto(byte goto_col, byte goto_row);
void Put_normal_video_end();
void Put_start_row(byte new_row);
void Put_clear_all();
void Put_clear_all_text();
void Put_insert_line(word num);
void Put_delete_line(byte num);
void Put_scroll(byte num_lines);
void Put_reverse_scroll();
void Put_scroll_region(byte first_row, byte last_row);
void Reset_scroll_region();

/*****     SETCMD.PLM     *****/
void S_cmnd();
byte Set_from_macro();

/*****     START.PLM     *****/

/*! Size of blocks in work-files. */
void Add_eof();
void Subtract_eof();
void Build_banner();
void Print_banner();
static void Clear_text();
byte Debug_option(byte ch);
void Alter_init();
void Flip_pointers();
void O_cmnd();
void Q_cmnd();
void Quit_exit(byte status);

/*****     TAGS.PLM     *****/
static void Delete_blocks(byte wk1_or_wk2, word ndelete);
void Set_tag(byte tagn, pointer in_mem);
void Set_tagi_at_lowe();
void Jump_tag(byte tagn);
void Jump_tagi();
void Clean_tags();
void Delete_to_tag(byte tagn);

/*****    TMPMAN.PLM    *****/

void Tmpman_init();

  /* initializes pointers for the 286 */

//boolean Handle_aftn_full();
void Reinit_temp(byte file_num);

/*!
   Frees all the blocks associated with that file, and resets
   associated variables. If the mode is not viewonly, the physical
   file is rewineded. */

void Set_temp_viewonly(byte file_num, byte mode, connection conn);
/*!
   Sets the mode of a workfile to view_only_wk1 (or wk2),
   and passes to it the connection of the input file to
   allow the special mechanism of view_only. */

void Read_temp(byte file_num, pointer buf_addr);
/*!
   Read the current block of virtual_files(file_num) and moves it to
   buf_addr. first reads are taken from memory, then it starts to
   read from the disk file if it was necessary to have spilled it to
   disk. The read blocks are not removed from the file.
   The current block's successor becomes the current block.
   No need to OPEN_IF_NECESSARY, because rewind does the job. */

logical Read_previous_block(byte file_num, pointer buf_addr, logical keep);
/*!
   Read the current block of virtual_files(file_num) and moves it to
   buf_addr. first reads are taken from memory, then it starts to
   read from the disk file if it was necessary to have spilled it to
   disk. If keep is false, the read blocks are removed from the file.
   The current block's predecessor becomes the current block. */

void Backup_temp(byte file_num);
/*!
   Remove the last block of the file. */

void Skip_to_end(byte file_num);
/*!
   Set the pointers such that we are at the end of the file.
   No need to OPEN_IF_NECESSARY, because read_previous_block does the job. */

void Rewind_temp(byte file_num);
/*!
   Set the pointers such that we are at the beginning of the file. */

void Restore_temp_state(byte file_num);
/*!
  The  procedure copy_wk1 causes vf.cur_block_ptr to be NIL.  In this
  situation  it is the caller's responsibility to recover the current
  setting, using this procedure. */

void Write_temp(byte file_num, pointer buf_addr);
/*!
   Write to the given file the data at buf_addr. Try to put in free
   memory first, otherwise, spill the oldest written data to disk,
   keeping the newer data in memory if possible. If there are no
   blocks in memory at all, spill the new block to disk. */


/*****     UTIL.PLM     *****/

/*! _IF VAX 
  time:PROCEDURE(x);
    DCL x word; END;
/*! _ENDIF  */
#define Min(a,b) ((a) < (b) ? (a) : (b))
#define Max(a,b) ((a) > (b) ? (a) : (b))

word Size_of_text_in_memory();
void Move_name(pointer fromp, pointer top);
byte Cmp_name(pointer fromp, pointer top);
pointer Print_number(dword d_number, byte b_width, byte base);
void Init_str(pointer str_p, byte max_len);
void Reuse_str(pointer str_p, byte max_len);
void Add_str_char(byte ch);
void Add_str_str(pointer str_p);
void Add_str_num(dword num, byte base);
void Init_scan(pointer str_p);
boolean Skip_char(byte ch);
word Num_in(boolean *err_p);

/*****     VIEW.PLM     *****/

void Movmem(pointer from_word, pointer to_word, word len);
void Re_window();
void Rebuild(byte r_row, pointer newp, pointer oldp);
void Save_line(byte s_row);
static pointer Unsave_line(byte u_row);
void Forward_char(word num_chars, byte delete_flag);
void Forward_line(word num_lines);
void Backup_char(word num_chars, byte delete_flag);
pointer Backup_line(int num_lines,pointer start, byte do_move);
pointer Print_text_line(pointer line);
void Re_write(pointer start);
void Re_view();

/*****     wordS.PLM     *****/
void P_cmnd();
void W_cmnd();
void K_cmnd();

/*****   AEDDUM.PLM   *****/

void Cocom(byte ch);             /* AEDASM */
void Codat(byte ch);             /* AEDASM */
byte Cidat();                    /* AEDASM */
void Send_block(pointer string);        /* AEDASM */
void Enable_ioc_io();            /* AEDASM */
void Disable_ioc_io();           /* AEDASM */
void Port_co(byte ch);           /* AEDASM */
byte Port_ci();                  /* AEDASM */
byte Port_csts();                /* AEDASM */
pointer Unfold_to_screen(pointer line); /* IOCASM */

/*****   XNXSYS.PLM   *****/
pointer Getenv(pointer symbol_p);

/*!****   INFACE   *****/
//address tgetstr_(pointer symbol_p);
//address tgoto_(pointer scrstr_p, word col, word row);
//word tgetent_(pointer buf_p, pointer name_p) word;
//word tgetnum_(pointer name_p) word;
//word tgetflag_(pointer name_p) word;
//void ignore_quit_signal();
//void handle_ioctl():

/*!****   TRMCAP   *****/
//void xenix_setup_terminal();
//address  interpret_cursor: PROC (byte goto_col, byte goto_row) address;


byte Cli_command();
void Co(byte byt);
void Construct_point(pointer buff_ptr, pointer point_ptr);
void Dealias_cmd(word index);
void Display_output(pointer buff_ptr);
void Excep_handler(word condition_code, word param_code, word reserved, word npx_status);
logical Find();
pointer Find_alias(pointer alias_str_ptr);
void Find_term_type(pointer buf_ptr);
void Get_access_rights(pointer path_p);
boolean Look_ahead(byte num);
void Put_access_rights(pointer path_p);
void Restore_input_line_state();
void Rq_c_delete_command_connection(word command_conn, wpointer except_ptr);
void Save_input_line_state();
void Swb();

void Term_config_process();
void To_upper(pointer src_ptr, pointer dest_ptr, word count);

/** UDI ***/
selector dq_allocate(word size, wpointer excep_p);
connection dq_attach(pointer path, wpointer excep_p);
void dq_change_access(pointer path_p, byte class, byte access, wpointer excep_p);
void dq_change_extension(pointer path_p, pointer extension_p, wpointer excep_p);
void dq_close(connection conn, wpointer excep_p);
connection dq_create(pointer path_p, wpointer excep_p);
void dq_decode_exception(word excep_code, pointer message_p, word maxLen);
void dq_decode_time(dt_t *dt_p, wpointer execpp);
void dq_delete(pointer path_p, wpointer excep_p);
void dq_detach(connection conn, wpointer excep_p);
void dq_file_info(word conn, byte mode, file_info_t *file_info_p,  wpointer excep_p);
void dq_free(selector mem_token, wpointer excep_p);
byte dq_get_argument(pointer argument_p, wpointer excep_p);
void dq_get_exception_handler(exception_handler_t *handler_p, wpointer excep_p);
word dq_get_size(selector mem_token, wpointer excep_p);
void dq_get_system_id(pointer id_p, wpointer excep_p);       // id_p -> byte buf[21]
void dq_get_time(old_dt_t *dt_p,  wpointer excep_p);         // obsolete
void dq_open(connection conn, byte mode, byte num_buf, wpointer excep_p);
void dq_overlay(pointer name_p, wpointer execp_p);
word dq_read(connection conn, pointer buf_p, word count, wpointer excep_p);
void dq_rename(pointer old_p, pointer new_p, wpointer excep_p);
void dq_reserve_io_memory(word number_files, word number_buffers, wpointer excep_p);
void dq_seek(connection conn, byte mode, dword offset, wpointer excep_p);
void dq_special(byte type, void *ptr_p, wpointer excepp);
word dq_switch_buffer(pointer buffer_p, wpointer excep_p);
void dq_trap_cc(void (* cc_procedure_p)(), wpointer excepp);
void dq_truncate(connection conn, wpointer excep_p);
void dq_write(connection conn,pointer buf_p, word count, wpointer excep_p);

/*! _IF VAX
void dq_set_delimiters(pointer delimiter_p, wpointer excep_p);
word dq_attach_first(pointer path, wpointer excep_p);
word dq_create_crlf(pointer path, wpointer excep_p);
/*! _ENDIF */

#define length(var) (sizeof(var)/sizeof(*var))
#define last(var)   (sizeof(var)/sizeof(*var) - 1)

word low(word val);
word high(word val);

#define highw(n)    (((n) >> 16) & 0xffff)
#define loww(n)     ((n) & 0xffff)



void movrb(pointer s, pointer t, word cnt);
word cmpb(pointer s, pointer t, byte cnt);
word cmpw(pointer s, pointer t, word cnt);

word findb(byte *str, byte ch, word cnt);
word findrb(byte *str, byte ch, word cnt);
word findp(pointer src[], pointer p, word cnt);
word skipb(byte *str, byte ch, word cnt);
word skiprb(byte *str, byte ch, word cnt);

void move(byte cnt, pointer s, pointer t);


#if 0       /* local procedures */
static void Actual_goto(goto_col,goto_row);
static void Add_ch(ch);
static void Add_in_quot();
static byte Add_input_buffer();
static void Add_param();
static void Add_three();
static void Add_to_message(addstr);
static void Add_two(two);
static void Alias(cmd_index);
static void Assign_to_message_is(newval);
static boolean At_crlf(str_p, i, left);
static boolean At_eof();
static void B_cleanup();
static void Back_char();
static byte Bad_hex(code_p, max_num, in_place);
static void Bad_value(p);
static void Block_block();
static void Block_controlc();
static void Block_delete();
static void Block_escape();
static void Block_put();
static pointer Build_ptr(token, offset);
static void Calc_error(err_msg);
static void Calc_statement();
static byte Can_add_macro();
static byte Char_subtype(ch);
static byte Char_type(ch);
static void Check();
statis boolean Check_if_quoted_string();
static void Check_stack_ovf();
static void Chop_blanks();
static void Close_block();
static byte Cmp_dw(num1,num2);
static byte Cmp_name_insensitively(fromp,top);
static void Collect_count();
static dword Collect_number();
static void Conflict_error();
static byte Control_type();
static dword Convert(base);
static byte Convert_if_cc(char);
static dword Convert_number();
static dword Convert_time_and_date(p);
static void Convert_xenix_format();
static void Copy_wk1_file(outfile);
static void Copy_wk2_file(outfile);
static byte Count_lines(fromp,top);
static void Crlf_and_indent();
static byte Cur_message_is();
static void Dealias(alias_ptr,former_flag);
static void Delay(func);
static void Display_alias_exp(alias_name_ptr,alias_exp_ptr);
static void Display_filenames();
static void Do_config_termcap();
static void Do_delete();
static void Do_delete_left();
static void Do_delete_line();
static void Do_delete_right();
static lvars Do_macro_file();
static void Do_replace(need_save);
static void Draw_first_at_sign(ch);
static dword Element();
static void Exchange_a_char(ch);
static dword Exp();
static dword Factor();
static void Fill_chars();
static boolean Find_bop();
static lvars Flush();
static void For_char();
static void Free_block(blk_p);
static void Get_arg();
static pointer Get_block();
static void Get_controls();
static byte Get_entry(src_ptr,dest_ptr,max_bytes);
static void Get_input_file_name();
static size Get_next_alias_param(alias_exp_ptr,prev_index);
static dword Get_offset(block_num,offset_in_block);
static address Get_quoted_string();
static word Get_string_variable(byte ch);
static void Init_alias_table(output_conn_in,co_conn_in,echo_in,excep_ptr);
static void Input_expected();
static byte Input_l(prompt_p,prev_string_p);
static byte Input_macro_name();
static void Input_param();
static void Insert_blank();
static void Insert_char(ch);
static void Insert_string(str);
static void Invocation_error();
static boolean Is_a_system_variable();
static boolean Is_assign();
static boolean Is_open(i);
static void Jump_end();
static void Jump_prior_boln();
static byte Keep_after_all();
static byte Keep_other_after_all();
static void Last_dispatch();
static void Link_to_end(file_num, blk_p);
static void Link_to_lru(blk_p);
static void List_tabs();
static dword Logicalops();
static byte Macro_char();
static void Macro_create();
static void Macro_get();
static void Macro_insert();
static void Macro_list();
static void Macro_null();
static word Macro_number();
static void Macro_save();
static void Move_cursor(ch);
static dword N_stat_in_parens();
static dword N_statement();
static byte Not_coded(ch);
static byte Not_equal();
static byte Not_semi_colon();
static boolean Null_line();
static byte On_screen();
static void Open_block(mode);
static void Open_if_necessary(file_num);
static void Parse_tail();
static dword Primary();
static void Print_code(code_p);
static void Print_counters();
static void Print_name(name_p);
static void Process_macro_comment();
static logical Process_line(start_col);
static void Process_t_or_f(flag_p,err_p);
static void Put_back();
static void Put_delete();
static void Put_down();
static void Put_indicator(ch);
static void Put_insert();
static void Put_re_row(new_row,want_col);
static void Put_right();
static void Put_up();
static void Q_cleanup();
static void Read_input_file();
static byte Real_pos();
static dword Relops();
static void Remove_slash();
static void Rename_as_bak();
static void Restore_state();
static boolean Reverse_scroll(num_line);
static word S_statement();
static void Save_state();
static byte Saved_in_block();
static void Scan();
static boolean Scroll(num_line);
static void Scroll_partial_down(insert_row,num);
static void Scroll_partial_up(num);
static void Select_partition(part_no);
static word Selecter_of(ptr);
static void Send_alias_command(conn_tok,buff_ptr,cmd_excep_ptr,excep_ptr);
static void Set_autocr();
static void Set_bak_file();
static void Set_case();
static void Set_display();
static void Set_e_delimit();
static void Set_error(msg);
static void Set_go();
static void Set_highbit();
static void Set_indent();
static byte Set_input_line(prompt,prev_input);
static void Set_input_yes_no(prompt_string,current_value_p);
static void Set_k_token();
static void Set_leftcol();
static void Set_margin();
static void Set_notab();
static void Set_pause();
static void Set_radix();
static void Set_showfind();
static void Set_tabs();
static void Set_viewrow();
static void Set_wrapcol();
static void Set_zone();
static void Setup_memory();
static byte Smaller(a,b);
static dword Srops();
static lvars Start_error(msg);
static void Switch_windows();
static void System_call();
static dword System_variable();
static dword Term();
static void Text_lost();
static void U_restore();
static void U_save();
static void U_setup();
static void Ubuf_char(ch);
static pointer Unlink_from_end(file_num);
static void Unlink_from_lru(blk_p);
static void Unlink_from_start(file_num);
static void Update();
static void V1_cmnd();
static void Wrap_macro();
static void Write_entire_file(nfile);
static void Write_to_tagi(nfile,do_delete);
static void Write_wk1();
static void Write_wk2(start);
static void Xbuf_clean();
static void Xx_controlc();
static void Xx_rubout();
#endif

