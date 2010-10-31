#ifndef EXTERNS
   #define EXTERNS

   /* Lex */
   extern char* input_msg;
   extern char* output_msg;
   extern char  id_stack[];

   extern int data_segment_offset;
   extern int id_stack_index;
   extern int num_bytes;
   extern int symbol_table_index;

   /* Yacc */
   extern int   code_stack_pos;
   extern int   fixup_stack_pos;
   extern int   num_bytes;

   extern FILE* yyin;

   /* common.c */
   extern unsigned char code_stack[];
   extern unsigned char fixup_stack[];
#endif
