#ifndef FUNCTIONS
   #define FUNCTIONS

   /******** Module functions ********/
   void create_object_module(char* filename, FILE* output_file);
   void create_threadr(char* filename, FILE* output_file);
   void create_lnames(FILE* output_file);
   void create_stack_segdef(FILE* output_file);
   void create_data_segdef(FILE* output_file);
   void create_code_segdef(FILE* output_file);
   void create_extdef(FILE* output_file);
   void create_ledata_data(FILE* output_file);
   /****************************************
   void create_ledata_code(FILE* output_file);
   void create_fixupp(FILE* output_file);
   ****************************************/
   void create_records(FILE* output_file);
   void create_modend(FILE* output_file);
   void check_code_stack_size();
   void fix_out(unsigned char* record, int record_pos, FILE* output_file);

   /***** Symbol table functions *****/
   int  find(char* name);
   int  hash(char* name);
   void make_entry(char* name, int j);

   /******** Helper functions ********/
   int  cmpstr(char* text1, char* text2);
   void add_exit_code();
   void fix_near_call(int index);
   void print_message();
   void print_char();
   void call_int21h();
   void print_identifier(int length, int name_index);
   void print_new_line();
   void reset_symbol_table();
   void set_ds_register();
   void call_near_func(int func_code);
   void compare_data(int location1, int location2);
   void add_far_jump_instruction(int instruction);
   void idiv_si();
   void move_imm_to_si(int number);
   void move_ax_to_from_si(char flag);
   void move_to_from_si(int offset, char flag);
   void move_to_from_ax(int offset, char flag);
   void add_mem_to_si(int offset);
   void sub_mem_from_si(int offset);
   void imul_si();
   void neg_si();
   void move_di_to_from_si(char flag);
   void get_word_length_and_column_number(int yytext_length, int yyleng_value); /* proj4.l */
#endif
