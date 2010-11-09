/**************************************************
**                                               **
** BJ Peter DeLaCruz                             **
** October 27, 2010                              **
** ICS 611                                       **
**                                               **
** Project #4                                    **
**                                               **
** This program will generate an object module   **
** with pairs of LEDATA and FIXUPP records given **
** a source code file. The object module can be  **
** used to create an executable file using the   **
** Link.exe program that comes with the MASM     **
** 6.15 assembler, which can be found here:      **
**                                               **
** http://fleder44.net/611/masm_615.ZIP          **
**                                               **
** See Features.txt for a list of features of    **
** this compiler.                                **
**                                               **
** To view information about the stacks used in  **
** this compiler, compile with the -DINFO flag.  **
**                                               **
**************************************************/

%{

    #include "proj4.h"

    typedef struct {
        int kind_of_location;
        int location;
    } yystype;
    #define   YYSTYPE   yystype

    typedef struct {
        int name_index;
        int offset;
        int hash_link;
        int length;
    } symbol_table_entry;

    extern symbol_table_entry symbol_table[];

    typedef struct lne {
        char identifier[IDENTIFIER_LENGTH];
        int line_number;
    } line_number_entry;

    extern line_number_entry line_number_table[];

    int  yylex();
    int  yyparse();
    void yyerror(char* mes);

    char* input_msg  = "Enter a value for $";
    char* output_msg = "The answer is $";

    int ledata_data_segment_index = 6;

    unsigned char ledata_data_segment[SIZE];
    unsigned char ledata_code_segment[SIZE];

    /*********************************************************************************
    ** The following are used to create one pair of LEDATA and FIXUPP records.      **
    **********************************************************************************
    ** int ledata_code_segment_index = 6;                                           **
    ** int fixupp_index              = 3;                                           **
    ** unsigned char fixupp[SIZE];                                                  **
    *********************************************************************************/
    unsigned char stack_segdef[] = {0x98, 0x07, 0x00, 0x74, 0x64, 0x00, 0x02, 0x05, 0x01, 0x81};

    /*********************************************************************************
    ** Replace positions 4 and 9 with segment length and checksum in data and code  **
    ** segdefs.                                                                     **
    *********************************************************************************/
    unsigned char data_segdef[] = {0x98, 0x07, 0x00, 0x48, 0x00, 0x00, 0x03, 0x06, 0x01, 0x00};
    unsigned char code_segdef[] = {0x98, 0x07, 0x00, 0x20, 0x00, 0x00, 0x04, 0x07, 0x01, 0x00};

    unsigned char modend[] = {0x8A, 0x06, 0x00, 0xC1, 0x50, 0x03, 0x00, 0x00, 0x5C};
    unsigned char extdef[] = {0x8C, 0x11, 0x00, 0x06, 'p', 'u', 't', 'd', 'e', 'c', 0x00, 0x06, 'g', 'e', 't', 'd', 'e', 'c', 0x00, 0xE6};

    /*********************************************************************************
    ** Assume that code segdef is listed third.                                     **
    *********************************************************************************/
    unsigned char ledata_code[SIZE]  = {0xA0,0x00,0x00,0x03,0x00,0x00};
    int ledata_code_pos              = 6;
    unsigned char fixup_record[SIZE] = {0x9C,0x00,0x00};
    int fixupp_pos                   = 3;

    int code_sizes_stack[SIZES_STACK_SIZE];
    int fixup_sizes_stack[SIZES_STACK_SIZE];
    /*********************************************************************************
    ** code_sizes_stack_pos and fixup_sizes_stack_pos are always equal.             **
    *********************************************************************************/
    int code_sizes_stack_pos = 0, fixup_sizes_stack_pos = 0;
    int prev_code_stack_pos  = 0, prev_fixup_stack_pos  = 0;
    int code_start           = 0, code_length           = 0;
    int fixup_start          = 0, fixup_length          = 0;

    /*********************************************************************************
    ** The following are used to create the error report.                           **
    *********************************************************************************/
    /********************************************************************************/
    extern int column_number;
    extern int word_length;

    char current_line[LINE_LENGTH];
    char source_code[NUMBER_OF_LINES][LINE_LENGTH];

    char  error_encountered     = FALSE;
    FILE* error_report          = NULL;
    int   line_number           = 0;
    int   number_of_errors      = 0;
    int   total_number_of_lines = 0;
    /********************************************************************************/

    char div_by_zero_encountered = FALSE;
    int  identifier_index        = 0;

    extern char string[];
    char is_string = FALSE;
%}

%token DO END ENDIF EQ GT GTE IDENTIFIER IF INPUT INT LT LTE MAIN NE NUMBER OUTPUT STEP STRING TO WEND WHILE
%%

program              : MAIN ';' declaration_list statement_list END MAIN ';'
                     | error ';'        { yyerrok; }
					 ;
declaration_list     : declaration ;
declaration          : INT identifier_list ';' ;
identifier_list      : identifier_list ',' IDENTIFIER | IDENTIFIER ;
statement_list       : statement_list statement | statement ;
statement            : input_statement ';'
                     | output_statement ';'
                     | assignment_statement ';'
                     | while_statement
                     | if_statement
                     | do_statement
                     | error ';'        { yyerrok; }
					 ;
input_statement      : INPUT IDENTIFIER {
                                          check_code_stack_size();
                                          common(LEA, DX, DIRECT, 0, ON);
                                          print_message();
                                          print_identifier(symbol_table[$2.location].length, symbol_table[$2.location].name_index);

                                          /*********************************************************************************
                                          ** Call getdec.                                                                 **
                                          *********************************************************************************/
                                          call_near_func(0x02);

                                          move_to_from_ax(symbol_table[$2.location].offset, OFF);
                                        }
                     ;
output_statement     : OUTPUT expression {
                                           /*********************************************************************************
                                           ** Print value of identifier, e.g. OUTPUT VAR1                                  **
                                           *********************************************************************************/
                                           if (!is_string) {
                                              check_code_stack_size();
                                              common(LEA, DX, DIRECT, CARRIAGE_RTN_LINE_FD + strlen(input_msg), ON);
                                              print_message();

                                              move_ax_to_from_si(ON);

                                              /*********************************************************************************
                                              ** Call putdec.                                                                 **
                                              *********************************************************************************/
                                              call_near_func(0x01);
                                              print_new_line();
                                           }
                                           /*********************************************************************************
                                           ** Print string (without quotation marks), e.g. OUTPUT "Hello World!"           **
                                           *********************************************************************************/
                                           else {
                                              int length = strlen(string);
                                              int pos    = 0;

                                              is_string = FALSE;
                                              print_new_line();
                                              for (pos = 0; pos < length; pos++) {
                                                  check_code_stack_size();
                                                  code_stack[code_stack_pos++] = 0xB2;
                                                  code_stack[code_stack_pos++] = string[pos];
                                                  check_code_stack_size();
                                                  print_char();
                                              }
                                              print_new_line();
                                           }
                                         }
                     ;
expression           : expression '+' IDENTIFIER {
                                                   /*********************************************************************************
                                                   ** Subtract the two identifiers if there is a unary operator present.           **
                                                   ** Otherwise, add the two identifiers together.                                 **
                                                   *********************************************************************************/
                                                   if ($3.kind_of_location == -1) {
                                                      sub_mem_from_si(symbol_table[$3.location].offset);
                                                   }
                                                   else {
                                                      add_mem_to_si(symbol_table[$3.location].offset);
                                                   }
                                                 }
                     | expression '+' NUMBER     {
                                                   move_di_to_from_si(ON);
                                                   move_imm_to_si($3.kind_of_location);
                                                   check_code_stack_size();
                                                   common(ADD, DI, SI, NO_DISPL, OFF);
                                                 }
                     | expression '-' IDENTIFIER {
                                                   /*********************************************************************************
                                                   ** Vice versa.                                                                  **
                                                   *********************************************************************************/
                                                   if ($3.kind_of_location == -1) {
                                                      add_mem_to_si(symbol_table[$3.location].offset);
                                                   }
                                                   else {
                                                      sub_mem_from_si(symbol_table[$3.location].offset);
                                                   }
                                                 }
                     | expression '-' NUMBER     {
                                                   move_di_to_from_si(ON);
                                                   move_imm_to_si($3.kind_of_location);
                                                   check_code_stack_size();
                                                   common(SUB, DI, SI, NO_DISPL, ON);
                                                   move_di_to_from_si(OFF);
                                                 }
                     | expression '*' IDENTIFIER {
                                                   move_to_from_ax(symbol_table[$3.location].offset, ON);
                                                   if ($3.kind_of_location == -1) {
                                                      neg_si();
                                                   }
                                                   imul_si();
                                                   /*********************************************************************************
                                                   ** Note that the product cannot be greater than 16-bit.                         **
                                                   *********************************************************************************/
                                                   move_ax_to_from_si(OFF);
                                                 }
                     | expression '*' NUMBER     {
                                                   move_ax_to_from_si(ON);
                                                   move_imm_to_si($3.kind_of_location);
                                                   imul_si();
                                                   /*********************************************************************************
                                                   ** Note that the product cannot be greater than 16-bit.                         **
                                                   *********************************************************************************/
                                                   move_ax_to_from_si(OFF);
                                                 }
                     | expression '/' IDENTIFIER {
                                                   move_ax_to_from_si(ON);
                                                   move_to_from_si(symbol_table[$3.location].offset, ON);
                                                   if ($3.kind_of_location == -1) {
                                                      neg_si();
                                                   }
                                                   idiv_si();
                                                   move_ax_to_from_si(OFF);
                                                 }
                     | expression '/' NUMBER     {
                                                   /*********************************************************************************
                                                   ** Check if denominator is not equal to zero. If it is, set                     **
                                                   ** div_by_zero_encountered flag to TRUE to prevent object module from being     **
                                                   ** produced. If it isn't, store the number in SI and then perform the division. **
                                                   *********************************************************************************/
                                                   if ($3.kind_of_location != 0) {
                                                      move_ax_to_from_si(ON);
                                                      move_imm_to_si($3.kind_of_location);
                                                      idiv_si();
                                                      move_ax_to_from_si(OFF);
                                                   }
                                                   else {
                                                      div_by_zero_encountered = TRUE;
                                                   }
                                                 }
                     | IDENTIFIER {
                                    move_to_from_si(symbol_table[$1.location].offset, ON);
                                    /*********************************************************************************
                                    ** If there is a unary minus in front of the identifier, negate it.             **
                                    *********************************************************************************/
                                    if ($1.kind_of_location == -1) {
                                       neg_si();
                                    }
                                  }
                     | STRING     {
                                    is_string = TRUE;
                                  }
                     | '(' expression ')'
                     ;
assignment_statement : IDENTIFIER '=' expression {
                                                   move_to_from_si(symbol_table[$1.location].offset, OFF);
                                                 }
                     | IDENTIFIER '=' NUMBER     {
                                                   move_imm_to_si($3.kind_of_location);
                                                   move_to_from_si(symbol_table[$1.location].offset, OFF);
                                                 }
                     ;
while_statement      : while_prefix statement_list WEND {
                                                          check_code_stack_size();
                                                          code_stack[code_stack_pos++] = 0xE9;
                                                          code_stack[code_stack_pos++] = $1.kind_of_location  - (code_stack_pos + 2);
                                                          code_stack[code_stack_pos++] = ($1.kind_of_location - (code_stack_pos + 2)) >> 8;
                                                          code_stack[$1.location] = code_stack_pos - ($1.location + 2);
                                                          code_stack[$1.location + 1] = (code_stack_pos - ($1.location + 2)) >> 8;
                                                        }
                     ;
while_prefix         : while condition {
                                         $$.kind_of_location = $1.kind_of_location;
                                         $$.location         = $2.location;
                                       }
                     ;
while                : WHILE {
                               $$.kind_of_location = code_stack_pos;
                             }
                     ;
condition            : expression EQ expression  {
                                                   /*********************************************************************************
                                                   ** If BX is not equal to CX, exit the loop.                                     **
                                                   *********************************************************************************/
                                                   compare_data($1.location, $3.location);
                                                   add_far_jump_instruction(0x85);
                                                   $$.location = code_stack_pos - 2;
                                                 }
                     | expression NE expression  {
                                                   /*********************************************************************************
                                                   ** If BX is equal to CX, exit the loop.                                         **
                                                   *********************************************************************************/
                                                   compare_data($1.location, $3.location);
                                                   add_far_jump_instruction(0x84);
                                                   $$.location = code_stack_pos - 2;
                                                 }
                     | expression GTE expression {
                                                   /*********************************************************************************
                                                   ** If BX is less than CX, exit the loop.                                        **
                                                   *********************************************************************************/
                                                   compare_data($1.location, $3.location);
                                                   add_far_jump_instruction(0x8C);
                                                   $$.location = code_stack_pos - 2;
                                                 }
                     | expression LTE expression {
                                                   /*********************************************************************************
                                                   ** If BX is greater than CX, exit the loop.                                     **
                                                   *********************************************************************************/
                                                   compare_data($1.location, $3.location);
                                                   add_far_jump_instruction(0x8F);
                                                   $$.location = code_stack_pos - 2;
                                                 }
                     | expression GT expression  {
                                                   /*********************************************************************************
                                                   ** If BX is less than or equal to CX, exit the loop.                            **
                                                   *********************************************************************************/
                                                   compare_data($1.location, $3.location);
                                                   add_far_jump_instruction(0x8E);
                                                   $$.location = code_stack_pos - 2;
                                                 }
                     | expression LT expression  {
                                                   /*********************************************************************************
                                                   ** If BX is greater than or equal to CX, exit the loop.                         **
                                                   *********************************************************************************/
                                                   compare_data($1.location, $3.location);
                                                   add_far_jump_instruction(0x8D);
                                                   $$.location = code_stack_pos - 2;
                                                 }
                     ;
if_statement         : if_prefix statement_list ENDIF {
                                                        code_stack[$1.location] = code_stack_pos - ($1.location + 2);
                                                        code_stack[$1.location + 1] = (code_stack_pos - ($1.location + 2)) >> 8;
                                                      }
                     ;
if_prefix            : IF condition {
                                      $$.location = $2.location;
                                    }
                     ;
do_statement         : part_1 statement_list END DO {
                                                      check_code_stack_size();
                                                      code_stack[code_stack_pos++] = 0xE9;
                                                      code_stack[code_stack_pos++] = $1.location  - (code_stack_pos + 2);
                                                      code_stack[code_stack_pos++] = ($1.location - (code_stack_pos + 2)) >> 8;
                                                      code_stack[$1.kind_of_location] = code_stack_pos - ($1.kind_of_location + 2);
                                                      code_stack[$1.kind_of_location + 1] = (code_stack_pos - ($1.kind_of_location + 2)) >> 8;
                                                    }
                     ;
part_1               : part_2 TO expression    {
                                                 /*********************************************************************************
                                                 ** x <= val3                                                                    **
                                                 *********************************************************************************/
                                                 compare_data(identifier_index, $3.location);
                                                 add_far_jump_instruction(0x8F);
                                                 $$.location = $1.location;
                                                 $$.kind_of_location = code_stack_pos - 2;
                                               }
                     ;
part_2               : part_3 STEP expression  {
                                                 /*********************************************************************************
                                                 ** x += val2                                                                    **
                                                 *********************************************************************************/
                                                 move_to_from_si(symbol_table[identifier_index].offset, ON);
                                                 add_mem_to_si(symbol_table[$3.location].offset);
                                                 move_to_from_si(symbol_table[identifier_index].offset, OFF);
                                                 code_stack[$1.location] = code_stack_pos - ($1.location + 2);
                                                 code_stack[$1.location + 1] = (code_stack_pos - ($1.location + 2)) >> 8;
                                                 $$.location = $1.location + 2;
                                               }
                     ;
part_3               : DO assignment_statement {
                                                 check_code_stack_size();
                                                 /*********************************************************************************
                                                 ** Generate a jump instruction to jump over the first increment. Note that      **
                                                 ** DO x = val1 STEP val2 TO val3 => for (x = val1; x <= val3; x += val2)        **
                                                 *********************************************************************************/
                                                 code_stack[code_stack_pos++] = 0xE9;
                                                 code_stack[code_stack_pos++] = 0x00;
                                                 code_stack[code_stack_pos++] = 0x00;
                                                 $$.location = code_stack_pos - 2;
                                                 identifier_index = $2.location;
                                               }
                     ;

%%

int main(int argc, char* argv[]) {
    char  current_line[LINE_LENGTH];
    char  object_module_filename[FILENAME_LENGTH];
    char  error_report_filename[FILENAME_LENGTH];
    char  cross_reference_list_filename[FILENAME_LENGTH];

    char* file_extension       = NULL;
    int   current_line_number  = 0;
    int   length               = 0;
    int   position             = 0;
    FILE* source_code_file     = NULL;
    FILE* object_module        = NULL;
    FILE* cross_reference_list = NULL;

    /*********************************************************************************
    ** Create output file for object module.                                        **
    *********************************************************************************/
    strcpy(object_module_filename, argv[1]);
    if ((file_extension = strstr(object_module_filename, ".")) != NULL) {
       *(file_extension + 1) = '\0';
       strcat(object_module_filename, "obj");
    }
    else {
       strcat(object_module_filename, ".obj");
    }
    object_module = fopen(object_module_filename, "wb");
    if (object_module == NULL) {
       printf("Unable to open %s for writing.\n", object_module_filename);
       exit(EXIT_FAILURE);
    }

    /*********************************************************************************
    ** Open source code file for parsing.                                           **
    *********************************************************************************/
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
       printf("Unable to open %s for reading.\n", argv[1]);
       exit(EXIT_FAILURE);
    }

    /*********************************************************************************
    ** Open source code file for syntax checking.                                   **
    *********************************************************************************/
    source_code_file = fopen(argv[1], "r");
    if (source_code_file == NULL) {
       printf("Unable to open %s for reading.\n", argv[1]);
       exit(EXIT_FAILURE);
    }

    /*********************************************************************************
    ** Create output file that will contain error report.                           **
    *********************************************************************************/
    strcpy(error_report_filename, argv[1]);
    if ((file_extension = strstr(error_report_filename, ".")) != NULL) {
       *(file_extension + 1) = '\0';
       strcat(error_report_filename, "rpt");
    }
    else {
       strcat(error_report_filename, ".rpt");
    }
    error_report = fopen(error_report_filename, "w");
    if (error_report == NULL) {
       printf("Unable to open %s for writing.\n", error_report_filename);
       exit(EXIT_FAILURE);
    }

    /*********************************************************************************
    ** Read in source code and add line numbers to the beginning of each line for   **
    ** error report.                                                                **
    *********************************************************************************/
    for (current_line_number = 0; current_line_number < NUMBER_OF_LINES; current_line_number++) {
        sprintf(source_code[current_line_number], "%d", current_line_number + 1);
        length = strlen(source_code[current_line_number]);
        for   (position = 0; position < length; position++) { strcat(source_code[current_line_number], " "); }
        while (position++ < NUMBER_OF_SPACES - length)      { strcat(source_code[current_line_number], " "); }
    }

    current_line_number = 0;
    while (fgets(current_line, sizeof(current_line), source_code_file) != NULL) {
       strcat(source_code[current_line_number++], current_line);
       total_number_of_lines++;
    }

    /*********************************************************************************
    ** Prepare error report.                                                        **
    *********************************************************************************/
    fprintf(error_report, "Error Report\n==================================================\n");
    fprintf(error_report, "Line No.   Source Code\n");
    fprintf(error_report, "--------------------------------------------------\n");

    reset_symbol_table();

    set_ds_register();

    /*********************************************************************************
    ** Parse source code.                                                           **
    *********************************************************************************/
    yyparse();

    add_exit_code();
    
    if (code_stack_pos > prev_code_stack_pos) {
       code_sizes_stack[code_sizes_stack_pos]   = code_stack_pos;
       ++code_sizes_stack_pos;
       fixup_sizes_stack[fixup_sizes_stack_pos] = fixup_stack_pos;
       ++fixup_sizes_stack_pos;
    }

    create_object_module(argv[1], object_module);

    /*********************************************************************************
    ** Display results of syntax check. If errors were found, delete object module. **
    *********************************************************************************/
    if (number_of_errors > 1) {
       fprintf(error_report, "\n%d syntax errors encountered.\n\n", number_of_errors);
       if (number_of_errors > 0) {
          printf("%d syntax errors encountered. Object module not created.\n\n", number_of_errors);
       }
    }
    else if (number_of_errors == 1) {
       fprintf(error_report, "\n1 syntax error encountered.\n\n");
       printf("1 syntax error encountered. Object module not created.\n\n");
    }
    else {
       fprintf(error_report, "\n0 syntax errors encountered.\n\n");
       if (!div_by_zero_encountered) {
          printf("0 syntax errors encountered.\n\nObject module created [%s].\n", object_module_filename);
       }
    }

    /*********************************************************************************
    ** Print number of errors to error report.                                      **
    *********************************************************************************/
    if (div_by_zero_encountered || number_of_errors >= 1) {
       if (div_by_zero_encountered) {
          if (number_of_errors == 0) {
             printf("\n");
          }
          fprintf(error_report, "Error: Division by 0 encountered in source code.\n\n");
          printf("Error: Division by 0 encountered in source code. Object module not created.\n\n");
       }
       if (remove(object_module_filename) != 0) {
          printf("Error deleting object module.");
          exit(EXIT_FAILURE);
       }
    }

    fclose(error_report);
    fclose(source_code_file);
    fclose(yyin);
    fclose(object_module);

    strcpy(cross_reference_list_filename, argv[1]);
    if ((file_extension = strstr(cross_reference_list_filename, ".")) != NULL) {
       *(file_extension + 1) = '\0';
       strcat(cross_reference_list_filename, "lst");
    }
    else {
       strcat(cross_reference_list_filename, ".lst");
    }
    cross_reference_list = fopen(cross_reference_list_filename, "wb");
    if (cross_reference_list == NULL) {
       printf("Unable to open %s for writing.\n", cross_reference_list_filename);
       exit(EXIT_FAILURE);
    }

    create_cross_reference_list(cross_reference_list, argv[1]);
    fclose(cross_reference_list);

    printf("Error report created [%s].\nCross-reference list created [%s].\n\n", error_report_filename, cross_reference_list_filename);

    #ifdef INFO
       printf("**************************************************\n");
       printf("Notes:\n   -- Numbers are in bytes.\n   -- Code sizes and fixup sizes stacks are used to create pairs of LEDATA and FIXUPP records.\n");
       printf("   -- Sizes of code sizes and fixup sizes stacks must be the same.\n\n");
       printf("Current size of code stack:        [%5d]\n", code_stack_pos);
       printf("Current size of fixup stack:       [%5d]\n", fixup_stack_pos);
       printf("Current size of code sizes stack:  [%5d]\n", code_sizes_stack_pos);
       printf("Current size of fixup sizes stack: [%5d]\n", fixup_sizes_stack_pos);
       printf("\n");
       printf("Maximum size of code stack:        [%5d] <-- CODE_STACK_SIZE\n", CODE_STACK_SIZE);
       printf("Maximum size of fixup stack:       [%5d] <-- FIXUP_STACK_SIZE\n", FIXUP_STACK_SIZE);
       printf("Maximum size of code sizes stack:  [%5d] <-- SIZES_STACK_SIZE\n", SIZES_STACK_SIZE);
       printf("Maximum size of fixup sizes stack: [%5d] <-- SIZES_STACK_SIZE\n", SIZES_STACK_SIZE);
       printf("\n");
       printf("Modify the four constants above to resize the respective stacks. The constants can be found in constants.h.\n");
       printf("**************************************************\n");
    #endif

    return EXIT_SUCCESS;
}

void yyerror(char *mes) {
    int position;

    if (line_number < total_number_of_lines) {
       /*********************************************************************************
       ** End of line was reached, so print the source code and an asterisk if there   **
       ** was an error in it.                                                          **
       *********************************************************************************/
       if (strcmp(mes, "eol") == 0) {
          fprintf(error_report, "%s", source_code[line_number]);
          if (error_encountered) {
             printf("%s", source_code[line_number]);
             error_encountered = FALSE;
          }
          line_number++;
       }
       else {
          /*********************************************************************************
          ** End of line was not reached because there was a syntax error in the source   **
          ** code.                                                                        **
          *********************************************************************************/
          for (position = 0; position < NUMBER_OF_SPACES; position++)            { strcat(source_code[line_number], " "); }
          for (position = 0; position < column_number - word_length; position++) { strcat(source_code[line_number], " "); }
          strcat(source_code[line_number], "*\n     Syntax error.\n\n");
          number_of_errors++;
          error_encountered = TRUE;
          if (line_number == 0) {
             fprintf(error_report, "%s", source_code[line_number]);
          }
       }
    }
}

/***************************************************
***************** Module functions *****************
***************************************************/

void create_object_module(char* filename, FILE* object_module) {
    create_threadr(filename, object_module);
    create_lnames(object_module);
    create_stack_segdef(object_module);
    create_data_segdef(object_module);
    create_code_segdef(object_module);
    create_extdef(object_module);
    create_ledata_data(object_module);
    /*********************************************************************************
    ** Create one pair of LEDATA and FIXUPP records.                                **
    **********************************************************************************
    ** create_ledata_code(object_module);                                           **
    ** create_fixupp(object_module);                                                **
    *********************************************************************************/

    /*********************************************************************************
    ** Create pairs of LEDATA and FIXUPP records.                                   **
    *********************************************************************************/
    create_records(object_module);

    create_modend(object_module);
}

void create_threadr(char* filename, FILE* object_module) {
    int length              = 0;
    unsigned char character = 0;
    unsigned char pos       = 0;
    unsigned int  checksum  = 0;

    character = 0x80;
    checksum += character;
    fputc(character, object_module);

    character = strlen(filename) + 2;
    checksum += character;
    fputc(character, object_module);

    character = 0x00;
    fputc(character, object_module);

    character = strlen(filename);
    checksum += character;
    fputc(character, object_module);

    length = strlen(filename);
    for (pos = 0; pos < length; pos++) {
        fputc(filename[pos], object_module);
        checksum += filename[pos];
    }

    character = 0 - checksum;
    fputc(character, object_module);
}

void create_lnames(FILE* object_module) {
    int length              = 0;
    unsigned char character = 0;
    unsigned char pos       = 0;
    unsigned int  checksum  = 0;
    char string[10];

    character = 0x96;
    checksum += character;
    fputc(character, object_module);

    character = 0x24;
    checksum += character;
    fputc(character, object_module);

    character = 0x00;
    fputc(character, object_module);
    fputc(character, object_module);

    character = 0x05;
    checksum += character;
    fputc(character, object_module);

    strcpy(string, "STACK");
    length = strlen(string);
    for (pos = 0; pos < length; pos++) {
        fputc(string[pos], object_module);
        checksum += string[pos];
    }

    character = 0x05;
    checksum += character;
    fputc(character, object_module);

    strcpy(string, "_DATA");
    length = strlen(string);
    for (pos = 0; pos < length; pos++) {
        fputc(string[pos], object_module);
        checksum += string[pos];
    }

    character = 0x05;
    checksum += character;
    fputc(character, object_module);

    strcpy(string, "_TEXT");
    length = strlen(string);
    for (pos = 0; pos < length; pos++) {
        fputc(string[pos], object_module);
        checksum += string[pos];
    }

    character = 0x05;
    checksum += character;
    fputc(character, object_module);

    strcpy(string, "STACK");
    length = strlen(string);
    for (pos = 0; pos < length; pos++) {
        fputc(string[pos], object_module);
        checksum += string[pos];
    }

    character = 0x04;
    checksum += character;
    fputc(character, object_module);

    strcpy(string, "DATA");
    length = strlen(string);
    for (pos = 0; pos < length; pos++) {
        fputc(string[pos], object_module);
        checksum += string[pos];
    }

    character = 0x04;
    checksum += character;
    fputc(character, object_module);

    strcpy(string, "CODE");
    length = strlen(string);
    for (pos = 0; pos < length; pos++) {
        fputc(string[pos], object_module);
        checksum += string[pos];
    }

    character = -checksum;
    fputc(character, object_module);
}

void create_stack_segdef(FILE* object_module) {
    int pos;

    for (pos = 0; pos < STACK_SEGDEF_LENGTH; pos++) {
        fputc(stack_segdef[pos], object_module);
    }
}

void create_data_segdef(FILE* object_module) {
    int pos               = 0;
    unsigned char byte    = 0xFF;
    unsigned int checksum = 0;
    unsigned int size     = 0;

    size = (num_bytes * WORD_SIZE) + CARRIAGE_RTN_LINE_FD + strlen(input_msg) + CARRIAGE_RTN_LINE_FD + strlen(output_msg);
    if (size > 0xFF) {
       byte &= size;
       data_segdef[4] = byte;
       byte = 0xFF;
       byte &= (size >> 8);
       data_segdef[5] = byte;
    }
    else {
       data_segdef[4] = num_bytes * WORD_SIZE;
       data_segdef[4] += CARRIAGE_RTN_LINE_FD + strlen(input_msg);
       data_segdef[4] += CARRIAGE_RTN_LINE_FD + strlen(output_msg);
    }

    for (pos = 0; pos < DATA_SEGDEF_LENGTH; pos++) {
        checksum += data_segdef[pos];
    }
    data_segdef[DATA_SEGDEF_LENGTH - 1] = -checksum;
    for (pos = 0; pos < DATA_SEGDEF_LENGTH; pos++) {
        fputc(data_segdef[pos], object_module);
    }
}

void create_code_segdef(FILE* object_module) {
    int pos               = 0;
    unsigned char byte    = 0xFF;
    unsigned int checksum = 0;
    unsigned int size     = 0;

    size = code_stack_pos;
    if (size > 0xFF) {
       byte &= size;
       code_segdef[4] = byte;
       byte = 0xFF;
       byte &= (size >> 8);
       code_segdef[5] = byte;
    }
    else {
       code_segdef[4] = code_stack_pos;
    }

    for (pos = 0; pos < CODE_SEGDEF_LENGTH; pos++) {
        checksum += code_segdef[pos];
    }
    code_segdef[CODE_SEGDEF_LENGTH - 1] = -checksum;
    for (pos = 0; pos < CODE_SEGDEF_LENGTH; pos++) {
        fputc(code_segdef[pos], object_module);
    }
}

void create_extdef(FILE* object_module) {
    int length = 20;
    int pos    = 0;

    for (pos = 0; pos < length; pos++) {
        fputc(extdef[pos], object_module);
    }
}

void create_ledata_data(FILE* object_module) {
    int length            = strlen(input_msg);
    int pos               = 0;
    unsigned char byte    = 0xFF;
    unsigned int checksum = 0xA0 + ((0x0A + 0x0D) * 2) + 0x02;
    unsigned int diff     = 0;
    unsigned int size     = 0;

    /****************************************************************
    ** Put input message in data segment.                          **
    ****************************************************************/
    ledata_data_segment[0] = 0xA0;
    ledata_data_segment[3] = 0x02;
    /****************************************************************
    ** 0x0A = carriage return, 0x0D = line feed                    **
    ****************************************************************/
    ledata_data_segment[ledata_data_segment_index++] = 0x0A;
    ledata_data_segment[ledata_data_segment_index++] = 0x0D;
    for (pos = 0; pos < length; pos++) {
        ledata_data_segment[ledata_data_segment_index++] = input_msg[pos];
        checksum += input_msg[pos];
    }

    /****************************************************************
    ** Put output message in data segment.                         **
    ****************************************************************/
    length = strlen(output_msg);
    ledata_data_segment[ledata_data_segment_index++] = 0x0A;
    ledata_data_segment[ledata_data_segment_index++] = 0x0D;
    for (pos = 0; pos < length; pos++) {
        ledata_data_segment[ledata_data_segment_index++] = output_msg[pos];
        checksum += output_msg[pos];
    }

    /****************************************************************
    ** Add 2-byte (word) variables, and initialize them to zero.   **
    /***************************************************************/
    for (pos = 0; pos < num_bytes; pos++) {
       ledata_data_segment[ledata_data_segment_index++] = 0x00;
       ledata_data_segment[ledata_data_segment_index++] = 0x00;
    }

    /****************************************************************
    ** Calculate length.                                           **
    ****************************************************************/
    size = ledata_data_segment_index - 2;
    if (size > 0xFF) {
       byte &= size;
       ledata_data_segment[1] = byte;
       byte = 0xFF;
       byte &= (size >> 8);
       ledata_data_segment[2] = byte;
    }
    else {
       ledata_data_segment[1] = ledata_data_segment_index - 2;
    }

    /****************************************************************
    ** Calculate checksum.                                         **
    ****************************************************************/
    diff = 0 - (checksum + ledata_data_segment[1] + ledata_data_segment[2]);
    ledata_data_segment[ledata_data_segment_index++] = diff;
    for (pos = 0; pos < ledata_data_segment_index; pos++) {
        fputc(ledata_data_segment[pos], object_module);
    }
}

/****************************************************************
void create_ledata_code(FILE* object_module) {
    int pos               = 0;
    unsigned char byte    = 0xFF;
    unsigned int checksum = 0xA0 + 0x03;
    unsigned int size     = 0;

    ledata_code_segment[0] = 0xA0;

    size = SEGDEF_NUMBER + OFFSET + code_stack_pos + CHECKSUM;
    if (size > 0xFF) {
       byte &= size;
       ledata_code_segment[1] = byte;
       byte = 0xFF;
       byte &= (size >> 8);
       ledata_code_segment[2] = byte;
    }
    else {
       ledata_code_segment[1] = SEGDEF_NUMBER + OFFSET + code_stack_pos + CHECKSUM;
    }
 
    ledata_code_segment[3] = 0x03;
    checksum += ledata_code_segment[1] + ledata_code_segment[2];
    for (pos = 0; pos < code_stack_pos; pos++) {
        ledata_code_segment[ledata_code_segment_index++] = code_stack[pos];
        checksum += code_stack[pos];
    }

    ledata_code_segment[ledata_code_segment_index++] = 0 - checksum;
    for (pos = 0; pos < ledata_code_segment_index; pos++) {
        fputc(ledata_code_segment[pos], object_module);
    }
}

void create_fixupp(FILE* object_module) {
    int pos               = 0;
    unsigned char byte    = 0xFF;
    unsigned int checksum = 0x9C;
    unsigned int size     = 0;

    fixupp[0] = 0x9C;

    size = fixup_stack_pos + CHECKSUM;
    if (size > 0xFF) {
       byte &= size;
       fixupp[1] = byte;
       byte = 0xFF;
       byte &= (size >> 8);
       fixupp[2] = byte;
    }
    else {
       fixupp[1] = fixup_stack_pos + CHECKSUM;
    }

    checksum += fixupp[1] + fixupp[2];
    for (pos = 0; pos < fixup_stack_pos; pos++) {
        fixupp[fixupp_index++] = fixup_stack[pos];
        checksum += fixup_stack[pos];
    }

    fixupp[fixupp_index++] = 0 - checksum;
    for (pos = 0; pos < fixupp_index; pos++) {
        fputc(fixupp[pos], object_module);
    }
}
****************************************************************/

void create_records(FILE* object_module) {
    int pos;

    for (pos = 0, code_start = 0, fixup_start = 0; pos < code_sizes_stack_pos; pos++) {
        code_length = code_sizes_stack[pos] - code_start;
        ledata_code[4] = code_start;
        ledata_code[5] = code_start >> 8;
        memmove(ledata_code + ledata_code_pos, code_stack + code_start, code_length);
        fix_out(ledata_code, ledata_code_pos + code_length, object_module);

        fixup_length = fixup_sizes_stack[pos] - fixup_start;
        memmove(fixup_record + fixupp_pos, fixup_stack + fixup_start, fixup_length);
        fix_out(fixup_record, fixupp_pos + fixup_length, object_module);

        code_start  = code_sizes_stack[pos]; 
        fixup_start = fixup_sizes_stack[pos];
    }
}

void create_modend(FILE* object_module) {
    int length = 9;
    int pos    = 0;

    for (pos = 0; pos < length; pos++) {
        fputc(modend[pos], object_module);
    }
}

void check_code_stack_size() {
   if (code_stack_pos - prev_code_stack_pos > 200) {
      code_sizes_stack[code_sizes_stack_pos++]   = code_stack_pos;
      fixup_sizes_stack[fixup_sizes_stack_pos++] = fixup_stack_pos;
      prev_code_stack_pos  = code_stack_pos;
      prev_fixup_stack_pos = fixup_stack_pos;
   }
}

void fix_out(unsigned char* record, int record_pos, FILE* object_module) {
    int checksum = 0;
    int pos      = 0;

    record[1] = record_pos - 2;        /* length field */
    record[2] = (record_pos - 2) >> 8; /* length field */
    for (pos = 0, checksum = 0; pos < record_pos; pos++)
        checksum += record[pos];
    record[record_pos] = -checksum;
    for (pos = 0; pos < record_pos + 1; pos++) {
        fputc(record[pos], object_module);
    }
}

/***************************************************
***************** Helper functions *****************
***************************************************/

int cmpstr(char* text1, char* text2) {
    char temp[1000];
    strncpy(temp, text1, strlen(text2));
    temp[strlen(text2)] = '\0';
    return (!strcmp(temp, text2));
}

void add_exit_code() {
    /****************************************************************
    ** MOV AX, 4C00h                                               **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB8;
    code_stack[code_stack_pos++] = 0x00;
    code_stack[code_stack_pos++] = 0x4C;
    call_int21h();
}

void fix_near_call(int index) {
    int size = code_stack_pos - prev_code_stack_pos - 2;

    /****************************************************************
    ** For use with getdec and putdec functions.                   **
    ****************************************************************/
    if (size < 0x0100) {
       fixup_stack[fixup_stack_pos++] = 0x84;
    }
    else if (size >= 0x0100 && size < 0x0200) {
       fixup_stack[fixup_stack_pos++] = 0x85;
    }
    else if (size >= 0x0200 && size < 0x0300) {
       fixup_stack[fixup_stack_pos++] = 0x86;
    }
    else if (size >= 0x0300) {
       fixup_stack[fixup_stack_pos++] = 0x87;
    }
    fixup_stack[fixup_stack_pos++] = size;
    fixup_stack[fixup_stack_pos++] = 0x06;
    fixup_stack[fixup_stack_pos++] = 0x03;
    fixup_stack[fixup_stack_pos++] = index;
}

void print_message() {
    /****************************************************************
    ** MOV AH, 09h                                                 **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB4;
    code_stack[code_stack_pos++] = 0x09;
    call_int21h();
}

void print_char() {
    /****************************************************************
    ** MOV AH, 02h                                                 **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB4;
    code_stack[code_stack_pos++] = 0x02;
    call_int21h();
}

void call_int21h() {
    /****************************************************************
    ** INT 21h                                                     **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xCD;
    code_stack[code_stack_pos++] = 0x21;
}

void print_identifier(int length, int name_index) {
    int pos;

    for (pos = name_index; pos < length + name_index; pos++) {
        /****************************************************************
        ** Put value of identifier in DL.                              **
        ****************************************************************/
        code_stack[code_stack_pos++] = 0xB2;
        code_stack[code_stack_pos++] = id_stack[pos];
        print_char();
    }
    /****************************************************************
    ** MOV DL, ';'                                                 **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB2;
    code_stack[code_stack_pos++] = 0x3A;
    print_char();
    /****************************************************************
    ** MOV DL, ' '                                                 **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB2;
    code_stack[code_stack_pos++] = 0x20;
    print_char();
}

void print_new_line() {
    /****************************************************************
    ** MOV DL, 0Ah                                                 **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB2;
    code_stack[code_stack_pos++] = 0x0A;
    print_char();
    /****************************************************************
    ** MOV DL, 0Dh                                                 **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB2;
    code_stack[code_stack_pos++] = 0x0D;
    print_char();
}

void reset_symbol_table() {
    int pos;

    for (pos = 0; pos < SIZE; pos++) {
        symbol_table[pos].hash_link  = -1;
        symbol_table[pos].offset     = -1;
        symbol_table[pos].name_index = -1;
        line_number_table[pos].line_number = -1;
    }
}

void set_ds_register() {
    /****************************************************************
    ** MOV AX, @data                                               **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0xB8;
    code_stack[code_stack_pos++] = 0x00;
    code_stack[code_stack_pos++] = 0x00;
    fixup_stack[fixup_stack_pos++] = 0xC8;
    fixup_stack[fixup_stack_pos++] = 0x01;
    fixup_stack[fixup_stack_pos++] = 0x54;
    fixup_stack[fixup_stack_pos++] = 0x02;
    /****************************************************************
    ** MOV DS, AX                                                  **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0x8E;
    code_stack[code_stack_pos++] = 0xD8;
}

void call_near_func(int func_code) {
    check_code_stack_size();
    code_stack[code_stack_pos++] = 0xE8;
    code_stack[code_stack_pos++] = 0x00;
    code_stack[code_stack_pos++] = 0x00;
    fix_near_call(func_code);
}

void compare_data(int location1, int location2) {
    check_code_stack_size();
    common(MOV, BX, DIRECT, symbol_table[location1].offset, ON);
    check_code_stack_size();
    common(MOV, CX, DIRECT, symbol_table[location2].offset, ON);
    /****************************************************************
    ** CMP BX, CX                                                  **
    ****************************************************************/
    code_stack[code_stack_pos++] = 0x3B;
    code_stack[code_stack_pos++] = 0xD9;
}

void add_far_jump_instruction(int instruction) {
    code_stack[code_stack_pos++] = 0x0F;
    code_stack[code_stack_pos++] = instruction;
    code_stack[code_stack_pos++] = 0x00;
    code_stack[code_stack_pos++] = 0x00;
}

void idiv_si() {
    /*********************************************************************************
    ** Sign-extend the signed value in AX to DX by using the instruction CWD.       **
    *********************************************************************************/
    check_code_stack_size();
    code_stack[code_stack_pos++] = 0x99;
    check_code_stack_size();
    code_stack[code_stack_pos++] = IDIV;
    code_stack[code_stack_pos++] = 0xFE;
}

void move_imm_to_si(int number) {
    check_code_stack_size();
    code_stack[code_stack_pos++] = 0xBE;
    code_stack[code_stack_pos++] = number;
    code_stack[code_stack_pos++] = number >> 8;
}

void move_ax_to_from_si(char flag) {
    check_code_stack_size();
    common(MOV, AX, SI, NO_DISPL, flag);
}

void move_to_from_si(int offset, char flag) {
    check_code_stack_size();
    common(MOV, SI, DIRECT, offset, flag);
}

void move_to_from_ax(int offset, char flag) {
    check_code_stack_size();
    common(MOV, AX, DIRECT, offset, flag);
}

void add_mem_to_si(int offset) {
    check_code_stack_size();
    common(ADD, SI, DIRECT, offset, ON);
}

void sub_mem_from_si(int offset) {
    check_code_stack_size();
    common(SUB, SI, DIRECT, offset, ON);
}

void imul_si() {
    check_code_stack_size();
    code_stack[code_stack_pos++] = 0xF7;
    code_stack[code_stack_pos++] = 0xEE;
}

void neg_si() {
    check_code_stack_size();
    code_stack[code_stack_pos++] = 0xF7;
    code_stack[code_stack_pos++] = 0xDE;
}

void move_di_to_from_si(char flag) {
    check_code_stack_size();
    common(MOV, DI, SI, NO_DISPL, flag);
}
