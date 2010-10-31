

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

/* tokens */

#define DO 257
#define END 258
#define ENDIF 259
#define EQ 260
#define GT 261
#define GTE 262
#define IDENTIFIER 263
#define IF 264
#define INPUT 265
#define INT 266
#define LT 267
#define LTE 268
#define MAIN 269
#define NE 270
#define NUMBER 271
#define OUTPUT 272
#define STEP 273
#define STRING 274
#define TO 275
#define WEND 276
#define WHILE 277

#ifndef yylval
YYSTYPE yylval;
#endif


#define YYDEBUG 1


int main(int argc, char* argv[]) {
    char  current_line[LINE_LENGTH];
    char  object_module_filename[FILENAME_LENGTH];
    char  error_report_filename[FILENAME_LENGTH];

    char* file_extension      = NULL;
    int   current_line_number = 0;
    int   length              = 0;
    int   position            = 0;
    FILE* source_code_file    = NULL;
    FILE* object_module       = NULL;

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
       strcat(error_report_filename, "lst");
    }
    else {
       strcat(error_report_filename, ".lst");
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
    fprintf(error_report, "Line No.   Source Code\n");
    fprintf(error_report, "------------------------------------------\n");

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
          printf("0 syntax errors encountered. Object module created [%s].\n", object_module_filename);
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

#define YYCONST const
typedef int yytabelem;

static YYCONST yytabelem yyfs[] = {
0, 0, 0, 0, 0, -2, 0, -3, 0, 0, 
-8, 0, 0, 0, -12, -13, -14, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, -33, 0, 0, 
0, -6, 0, -7, -9, -10, -11, -15, -16, 0, 
-26, -27, 0, 0, 0, 0, 0, -32, 0, -41, 
0, 0, -45, -4, 0, 0, 0, 0, 0, 0, 
0, -30, 0, -31, -40, 0, 0, 0, 0, 0, 
0, 0, 0, 0, -5, -1, -22, -23, -18, -19, 
-20, -21, -24, -25, -28, -42, 0, 0, 0, 0, 
0, 0};

static YYCONST yytabelem yyptbltok[] = {
269, 256, -1, 0, 59, 59, 266, -2, -3, -10000001, 
256, 263, 265, 272, 277, 264, 257, -5, -6, -7, 
-8, -10, -11, -12, -13, -15, -16, -17, -18, -19, 
-20, -10000001, 263, -4, 258, 256, 263, 265, 272, 277, 
264, 257, -6, -7, -8, -10, -11, -12, -13, -15, 
-16, -17, -18, -19, -20, -10000001, 59, 59, 59, -10000001, 
-10000001, -10000001, 59, 263, 263, 274, 40, -9, 61, 256, 
263, 265, 272, 277, 264, 257, -5, -6, -7, -8, 
-10, -11, -12, -13, -15, -16, -17, -18, -19, -20, 
256, 263, 265, 272, 277, 264, 257, -5, -6, -7, 
-8, -10, -11, -12, -13, -15, -16, -17, -18, -19, 
-20, 256, 263, 265, 272, 277, 264, 257, -5, -6, 
-7, -8, -10, -11, -12, -13, -15, -16, -17, -18, 
-19, -20, 263, 274, 40, -9, -14, 263, 274, 40, 
-9, -14, 275, -10000001, 273, 263, -10, 59, 44, -10000001, 
269, -10000001, -10000001, -10000001, -10000001, -10000001, -10000001, 59, 43, 45, 
42, 47, -10000001, -10000001, 263, 274, 40, -9, 263, 271, 
274, 40, -9, 256, 263, 265, 272, 276, 277, 264, 
257, -6, -7, -8, -10, -11, -12, -13, -15, -16, 
-17, -18, -19, -20, 256, 263, 265, 272, 277, 259, 
264, 257, -6, -7, -8, -10, -11, -12, -13, -15, 
-16, -17, -18, -19, -20, 258, 256, 263, 265, 272, 
277, 264, 257, -6, -7, -8, -10, -11, -12, -13, 
-15, -16, -17, -18, -19, -20, -10000001, 43, 45, 42, 
47, 260, 270, 262, 268, 261, 267, -10000001, 263, 274, 
40, -9, 263, 274, 40, -9, -10000001, -10000001, 263, 59, 
263, 271, 263, 271, 263, 271, 263, 271, 43, 45, 
42, 47, 41, -10000001, 59, 43, 45, 42, 47, 273, 
-10000001, -10000001, 257, 263, 274, 40, -9, 263, 274, 40, 
-9, 263, 274, 40, -9, 263, 274, 40, -9, 263, 
274, 40, -9, 263, 274, 40, -9, 256, 263, 265, 
272, 43, 45, 42, 47, 277, 264, 257, 43, 45, 
42, 47, 275, -10000001, -10000001, -10000001, -10000001, -10000001, -10000001, -10000001, 
-10000001, -10000001, -10000001, -10000001, -10000001, 256, 263, 265, 272, 43, 
45, 42, 47, 277, 264, 257, 256, 263, 265, 272, 
43, 45, 42, 47, 277, 264, 257, 256, 263, 265, 
272, 43, 45, 42, 47, 277, 264, 257, 256, 263, 
265, 272, 43, 45, 42, 47, 277, 264, 257, 256, 
263, 265, 272, 43, 45, 42, 47, 277, 264, 257, 
256, 263, 265, 272, 43, 45, 42, 47, 277, 264, 
257, -10000000};

static YYCONST yytabelem yyptblact[] = {
2, 3, 1, 0, 4, 5, 8, 6, 7, -2, 
17, 20, 18, 19, 27, 25, 29, 9, 10, 11, 
12, 13, 14, 21, 24, 15, 22, 16, 23, 26, 
28, -3, 31, 30, 32, 17, 20, 18, 19, 27, 
25, 29, 33, 11, 12, 13, 14, 21, 24, 15, 
22, 16, 23, 26, 28, -8, 34, 35, 36, -12, 
-13, -14, 37, 38, 40, 41, 42, 39, 43, 17, 
20, 18, 19, 27, 25, 29, 44, 10, 11, 12, 
13, 14, 21, 24, 15, 22, 16, 23, 26, 28, 
17, 20, 18, 19, 27, 25, 29, 45, 10, 11, 
12, 13, 14, 21, 24, 15, 22, 16, 23, 26, 
28, 17, 20, 18, 19, 27, 25, 29, 46, 10, 
11, 12, 13, 14, 21, 24, 15, 22, 16, 23, 
26, 28, 40, 41, 42, 48, 47, 40, 41, 42, 
48, 49, 50, -33, 51, 20, 52, 53, 54, -6, 
55, -7, -9, -10, -11, -15, -16, -17, 57, 58, 
56, 59, -26, -27, 40, 41, 42, 60, 40, 61, 
41, 42, 62, 17, 20, 18, 19, 63, 27, 25, 
29, 33, 11, 12, 13, 14, 21, 24, 15, 22, 
16, 23, 26, 28, 17, 20, 18, 19, 27, 64, 
25, 29, 33, 11, 12, 13, 14, 21, 24, 15, 
22, 16, 23, 26, 28, 65, 17, 20, 18, 19, 
27, 25, 29, 33, 11, 12, 13, 14, 21, 24, 
15, 22, 16, 23, 26, 28, -32, 57, 58, 56, 
59, 66, 71, 68, 70, 67, 69, -41, 40, 41, 
42, 72, 40, 41, 42, 73, -45, -4, 74, 75, 
76, 77, 78, 79, 80, 81, 82, 83, 57, 58, 
56, 59, 84, -30, -29, 57, 58, 56, 59, -29, 
-31, -40, 85, 40, 41, 42, 86, 40, 41, 42, 
87, 40, 41, 42, 88, 40, 41, 42, 89, 40, 
41, 42, 90, 40, 41, 42, 91, -43, -43, -43, 
-43, 57, 58, 56, 59, -43, -43, -43, 57, 58, 
56, 59, -44, -5, -1, -22, -23, -18, -19, -20, 
-21, -24, -25, -28, -42, -34, -34, -34, -34, 57, 
58, 56, 59, -34, -34, -34, -38, -38, -38, -38, 
57, 58, 56, 59, -38, -38, -38, -36, -36, -36, 
-36, 57, 58, 56, 59, -36, -36, -36, -39, -39, 
-39, -39, 57, 58, 56, 59, -39, -39, -39, -37, 
-37, -37, -37, 57, 58, 56, 59, -37, -37, -37, 
-35, -35, -35, -35, 57, 58, 56, 59, -35, -35, 
-35, -10000000};

static YYCONST yytabelem yyrowoffset[] = {
0, 3, 4, 5, 6, 9, 10, 31, 32, 34, 55, 56, 
57, 58, 59, 60, 61, 62, 63, 64, 68, 69, 
90, 111, 132, 137, 142, 143, 144, 145, 147, 149, 
150, 151, 152, 153, 154, 155, 156, 157, 162, 163, 
164, 168, 173, 194, 215, 236, 237, 247, 248, 252, 
256, 257, 258, 259, 260, 262, 264, 266, 268, 273, 
274, 280, 281, 282, 283, 287, 291, 295, 299, 303, 
307, 318, 323, 324, 325, 326, 327, 328, 329, 330, 
331, 332, 333, 334, 335, 346, 357, 368, 379, 390, 
401};

static YYCONST yytabelem yyr1[] = {
     0,    -1,    -1,    -2,    -3,    -4,    -4,    -5,    -5,    -6,
    -6,    -6,    -6,    -6,    -6,    -6,    -7,    -8,    -9,    -9,
    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,    -9,   -10,
   -10,   -11,   -12,   -13,   -14,   -14,   -14,   -14,   -14,   -14,
   -15,   -16,   -17,   -18,   -19,   -20};
static YYCONST yytabelem yyr2[] = {
     0,    14,     5,     2,     6,     6,     2,     4,     2,     4,
     4,     4,     2,     2,     2,     5,     5,     5,     7,     7,
     7,     7,     7,     7,     7,     7,     3,     3,     6,     7,
     7,     7,     5,     3,     7,     7,     7,     7,     7,     7,
     7,     5,     9,     7,     7,     5};

#ifdef YYDEBUG

typedef struct {char *t_name; int t_val;} yytoktype;

yytoktype yynts[] = {
	"program",	-1,
	"declaration_list",	-2,
	"declaration",	-3,
	"identifier_list",	-4,
	"statement_list",	-5,
	"statement",	-6,
	"input_statement",	-7,
	"output_statement",	-8,
	"expression",	-9,
	"assignment_statement",	-10,
	"while_statement",	-11,
	"while_prefix",	-12,
	"while",	-13,
	"condition",	-14,
	"if_statement",	-15,
	"if_prefix",	-16,
	"do_statement",	-17,
	"part_1",	-18,
	"part_2",	-19,
	"part_3",	-20,
	"-unknown-", 1  /* ends search */
};
yytoktype yytoks[] = {
	"DO",	257,
	"END",	258,
	"ENDIF",	259,
	"EQ",	260,
	"GT",	261,
	"GTE",	262,
	"IDENTIFIER",	263,
	"IF",	264,
	"INPUT",	265,
	"INT",	266,
	"LT",	267,
	"LTE",	268,
	"MAIN",	269,
	"NE",	270,
	"NUMBER",	271,
	"OUTPUT",	272,
	"STEP",	273,
	"STRING",	274,
	"TO",	275,
	"WEND",	276,
	"WHILE",	277,
	";",	59,
	",",	44,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"(",	40,
	")",	41,
	"=",	61,
	"-unknown-", -1  /* ends search */
};
char * yyreds[] = {
	"-no such reduction-"
	"program : MAIN ';' declaration_list statement_list END MAIN ';'", 
	"program : error ';'", 
	"declaration_list : declaration", 
	"declaration : INT identifier_list ';'", 
	"identifier_list : identifier_list ',' IDENTIFIER", 
	"identifier_list : IDENTIFIER", 
	"statement_list : statement_list statement", 
	"statement_list : statement", 
	"statement : input_statement ';'", 
	"statement : output_statement ';'", 
	"statement : assignment_statement ';'", 
	"statement : while_statement", 
	"statement : if_statement", 
	"statement : do_statement", 
	"statement : error ';'", 
	"input_statement : INPUT IDENTIFIER", 
	"output_statement : OUTPUT expression", 
	"expression : expression '+' IDENTIFIER", 
	"expression : expression '+' NUMBER", 
	"expression : expression '-' IDENTIFIER", 
	"expression : expression '-' NUMBER", 
	"expression : expression '*' IDENTIFIER", 
	"expression : expression '*' NUMBER", 
	"expression : expression '/' IDENTIFIER", 
	"expression : expression '/' NUMBER", 
	"expression : IDENTIFIER", 
	"expression : STRING", 
	"expression : '(' expression ')'", 
	"assignment_statement : IDENTIFIER '=' expression", 
	"assignment_statement : IDENTIFIER '=' NUMBER", 
	"while_statement : while_prefix statement_list WEND", 
	"while_prefix : while condition", 
	"while : WHILE", 
	"condition : expression EQ expression", 
	"condition : expression NE expression", 
	"condition : expression GTE expression", 
	"condition : expression LTE expression", 
	"condition : expression GT expression", 
	"condition : expression LT expression", 
	"if_statement : if_prefix statement_list ENDIF", 
	"if_prefix : IF condition", 
	"do_statement : part_1 statement_list END DO", 
	"part_1 : part_2 TO expression", 
	"part_2 : part_3 STEP expression", 
	"part_3 : DO assignment_statement", 
};
#endif /* YYDEBUG */

/*
 * Copyright (c) 2007, Xin Chen
 * All rights reserved. This file is distributed under BSD license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY the copyright holder ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL the copyright holder BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * hyaccpar
 *
 * HYACC Parser engine.
 *
 * @Author: Xin Chen
 * @Created on: 1/30/2007
 * @Last modified: 10/27/2007
 * @Copyright (C) 2007
 */

#define YYLEX() yylex()

#ifndef YYSTYPE
#define YYSTYPE int
#endif

#define YYNOACTION -10000000 
#define YYEOF     0   /* for strEnd, input end marker. */
#define YYERRCODE 256 /* for use by "error" token. */

#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
#define YYRESUME return(0)
#define YYABORT return(1)
#define YYACCEPT return(0)
#define YYERROR goto yyerrlab
#define YYRECOVERING() (!!yyerrflag) /* !! */
#define YYMAX_STACK_CAPACITY 16384   /* 2^14 */
int yystack_capacity = 256; /* initial stack capacity. */
#define YYNEW(type) (type *) malloc((yystack_capacity) * sizeof(type))
#define YYENLARGE(from, type) \
        (type *) realloc((void *) from, (yystack_capacity) * sizeof(type))
#define YYERR_EXIT(errmsg) { printf("%s\n", errmsg); exit(1); }

int yychar;     /* current input token number, i.e., lookahead. */
FILE * yyparse_fp; /* output file to trace parse steps. */
char * yyparse_file = "y.parse";
int * yyps;     /* state stack. */
int yyps_pt;    /* top of state stack: yyps+yyps_pt-1. */
YYSTYPE * yypv; /* value stack. */
int yypv_pt;    /* top of value stack: yypv+yypv_pt-1. */
#if YYDEBUG
int * yypm;     /* symbol stack. */
int yypm_pt;    /* top of symbol stack: yypm+yypm_pt-1. */
#endif

int yynerrs;    /* number of errors */
int yyerrflag;  /* error recovery flag */

extern int yychar;
extern int yyerrflag;

YYSTYPE yylval; /* the value of yylval is from yylex. */
YYSTYPE yyval;  /* "$$" used in production action. */


#if YYDEBUG

/* get token name from value of yychar. */
char * yyget_tok(const int yychar) {
  static char c[1];
  int i;
  if (yychar == YYEOF) return "EOF";
  if (yychar == 256) return "error";
  if (yychar < 0) { /* search for non-terminal. */
    for (i = 0; yynts[i].t_val <= 0; i ++)  
      if (yychar == yynts[i].t_val) return yynts[i].t_name;
  }
  /* yychar > 256, search for terminal. */
  for (i = 0; yytoks[i].t_val >= 0; i ++) 
    if (yychar == yytoks[i].t_val) return yytoks[i].t_name;
  if (yychar > 0 && yychar < 256) { c[0] = yychar; return c; }
  
  return "-none-";
}

void yywrite_stack() { 
  int i; 
  /* It's unknown what data type yypv is, so can't write here. */ 
  /* The user can customize the code here himself. */ 
  /* fprintf(yyparse_fp, "value stack: "); 
  for (i = 0; i < yypv_pt - 1; i ++) 
    fprintf(yyparse_fp, "%d, ", yypv[i]); 
  if (yypv_pt > 0) fprintf(yyparse_fp, "%d", yypv[i]); 
  fprintf(yyparse_fp, "\n"); 
  */

  fprintf(yyparse_fp, "symbol stack: ");
  for (i = 0; i < yypm_pt - 1; i ++)
    fprintf(yyparse_fp, "%s, ", yyget_tok(yypm[i]));
  if (yypm_pt > 0) fprintf(yyparse_fp, "%s", yyget_tok(yypm[i]));
  fprintf(yyparse_fp, "\n");
  
  fprintf(yyparse_fp, "state stack: "); 
  for (i = 0; i < yyps_pt - 1; i ++) 
    fprintf(yyparse_fp, "%d, ", yyps[i]); 
  if (yyps_pt > 0) fprintf(yyparse_fp, "%d", yyps[i]); 
  fprintf(yyparse_fp, "\n"); 
}

#define YYEXPAND_SYMBOL_STACK  \
    if ((yypm = YYENLARGE(yypm, int)) == NULL)  \
      YYERR_EXIT("YYEXPAND_STACK error: out of memory\n");

#else
#define YYEXPAND_SYMBOL_STACK
#endif


#define YYEXPAND_STACK \
{ \
  if (yyps_pt >= YYMAX_STACK_CAPACITY) { \
    printf("YYEXPAND_STACK error: YYMAX_STACK_CAPACITY reached.\n"); \
    exit(1); \
  } \
  if (yyps_pt >= yystack_capacity) { \
    yystack_capacity *= 2; \
    if ((yyps = YYENLARGE(yyps, int)) == NULL) \
      YYERR_EXIT("YYEXPAND_STACK error: out of memory\n"); \
    if ((yypv = YYENLARGE(yypv, YYSTYPE)) == NULL) \
      YYERR_EXIT("YYEXPAND_STACK error: out of memory\n"); \
    YYEXPAND_SYMBOL_STACK \
    /*printf("stack size expanded to %d\n", yystack_capacity); */ \
  } \
}


/*
 * A macro to get an action from (state, token) pair in the
 * parsing table. Use macro instead of function to improve
 * performance.
 * Input: state, token
 * Output: action
 * if action > 0, then is a shift/goto target state.
 * if action < 0, then is the reduction rule number.
 * if action == 0, then is accept.
 * if action == YYNOACTION, then no action exists.
 */
#define YYGET_ACTION(state, token, action) \
{ \
  /* offset_h to (offset_t - 1) is the range to search. */ \
  int offset_h = yyrowoffset[state]; \
  int offset_t = yyrowoffset[state + 1]; \
  \
  /* now use linear search. Will change to binary search. */ \
  int offset; \
  for (offset = offset_h; offset < offset_t; offset ++) \
    if (yyptbltok[offset] == token) break; \
  \
  if (offset == offset_t) action = YYNOACTION; \
  else action = yyptblact[offset]; \
}


/*
 * Handles error when no action is found in yyparse().
 * state: yystate. lookahead: yychar.
 * return: 0 if success, -1 if fail, 1 if eat a token.
 */
int yyerror_handler(int yystate) {
  int yyaction;
  switch (yyerrflag) {
    case 0:
      yyerror("syntax error"); 
#if YYDEBUG
      fprintf(yyparse_fp, "syntax error: \
        no action exists for state/token pair (%d, %s).\n",
        yystate, yyget_tok(yychar)); 
#endif
      goto skip_init;
    yyerrlab: /* we have a user generated syntax type error */
    skip_init:
      yynerrs ++;
    case 1:
    case 2: /* incompletely recovered error */
      yyerrflag = 3; /* ! */
      /* find state where "error" is a legal shift action */
      while (yyps_pt > 0) { /* while state stack is not empty. */
#if YYDEBUG
        fprintf(yyparse_fp, 
                "look for error action on state %d\n", yystate);
#endif
        YYGET_ACTION(yystate, YYERRCODE, yyaction);
        if (yyaction > 0) { /* shift on "error" found for yystate. */
          /* simulate shift for "error" token. */
          /* push target state on state stack. */
          * (yyps + yyps_pt) = yyaction; 
          if (++ yyps_pt == yystack_capacity) YYEXPAND_STACK; 
          * (yypv + yypv_pt) = yyval; /* should not matter */
          yypv_pt ++;
#if YYDEBUG
          * (yypm + yypm_pt) = YYERRCODE;
          yypm_pt ++;
          fprintf(yyparse_fp, "- shift on error\n");
#endif
          return 0; /* return control to yyparse(), resume parsing. */
        } else {    /* no error shift action found, pop this state. */
#if YYDEBUG
          fprintf(yyparse_fp, 
                  "- pop state %d\n", * (yyps + yyps_pt - 1));
#endif
          yyps_pt --; /* pop state stack. */
          yypv_pt --; /* pop value stack. */
#if YYDEBUG
          yypm_pt --;
          yywrite_stack();
#endif
          yystate = * (yyps + yyps_pt - 1); /* get current state. */
        }
      }
      /* the state stack is empty now, no error shift action found. */
#if YYDEBUG
      fprintf(yyparse_fp, "state stack is empty. abort.\n");
#endif
      return -1; /* yyparse return 1. */
    case 3: /* no shift yet, eat a token */
      if (yychar == YYEOF) return -1; /* yyparse() ABORT. */
#if YYDEBUG
      fprintf(yyparse_fp, "eat token %s\n", yyget_tok(yychar));
#endif
      /* discard lookahead, resume parsing. */
      yychar = -1; /* eat this token and read next symbol. */
      return 1; 
    default:
      YYERR_EXIT("yyerror_handler error: \
                 yyerrflag > 3 (should be 0~3)\n");
  } /* end of switch. */
} /* end of yyerror_handler(). */


/*
 * yyparse - return 0 if succeeds, 1 if anything goes wrong.
 */
int yyparse() {
  YYSTYPE * yypvt;    /* top of value stack for $vars. */
  int yystate, yyaction = -1, yy_lhs, yy_rhs_ct, yyerr_hdl;
  yynerrs = yyerrflag = 0;

#if YYDEBUG
  if ((yyparse_fp = fopen(yyparse_file, "w")) == NULL) 
    YYERR_EXIT("yyparse error: cannot open file y.parse\n");
   
  if ((yypm = YYNEW(int)) == NULL) 
    YYERR_EXIT("yyparse error: out of memory\n");
  
  yypm_pt = 0;
#endif
  if ((yyps = YYNEW(int)) == NULL || (yypv = YYNEW(YYSTYPE)) == NULL) 
    YYERR_EXIT("yyparse error: out of memory\n");

  yyps_pt = yypv_pt = 0;

  * (yyps + yyps_pt) = 0; /* push 0 onto the state stack. */
  if (++ yyps_pt == yystack_capacity) YYEXPAND_STACK; 

  yychar = -1; /* to allow reading the first symbol. */

  while (1) {
#if YYDEBUG
    yywrite_stack();
#endif

    if (yyaction >= 0 && yyfs[yyaction] < 0) {
      yyaction = yyfs[yyaction]; /* final state default reduce. */

    } else {
      if (yychar < 0) { /* we want to read next symbol. */
        yychar = yylex();
        if (yychar <= 0) yychar = YYEOF; /* end of file. */
#if YYDEBUG
        fprintf(yyparse_fp, "- read next symbol: %s\n", yyget_tok(yychar));
#endif
      }

      /* update current state: yystate. lookahead is yychar. */
      yystate = * (yyps + yyps_pt - 1);
      /* find action in parsing table. */
      YYGET_ACTION(yystate, yychar, yyaction);
#if YYDEBUG
      fprintf(yyparse_fp, "action at (%d, %s) is %d\n", 
              yystate, yyget_tok(yychar), yyaction);
#endif
    } /* end of else */


    if (yyaction > 0) { /* is shift */
      * (yyps + yyps_pt) = yyaction; /* push target state on state stack */
      if (++ yyps_pt == yystack_capacity) YYEXPAND_STACK; 

      yyval = yylval; /* yylval is obtained from yylex(). */
      * (yypv + yypv_pt) = yyval; /* push value onto value stack. */
      yypv_pt ++;

#if YYDEBUG
      * (yypm + yypm_pt) = yychar;
      yypm_pt ++;
      fprintf(yyparse_fp, "- shift: state %d. \n", yyaction);
#endif

      yychar = -1; /* we want to read next symbol. */
      if (yyerrflag > 0) yyerrflag --; 

    } else if (yyaction == YYNOACTION) { /* no action found. error */
      yyerr_hdl = yyerror_handler(yystate); 
      if (yyerr_hdl == -1) break; /* YYABORT; */
      else if (yyerr_hdl == 1) continue; /* eat a token. */

    } else if (yyaction < 0) { /* is reduction */
      yyaction = (-1) * yyaction; /* get reduction number. */
#if YYDEBUG
      fprintf(yyparse_fp, "- reduce: by rule %d. ", yyaction);
#endif

      yy_lhs = yyr1[yyaction]; /* lhs symbol. */
      yy_rhs_ct = yyr2[yyaction] >> 1; /* number of rhs symbols. */

      yypvt = yypv + yypv_pt - 1; /* top of value stack. */
      /* default: $$ = $1. $$ then can be changed by rule actions. */
      yyval = * (yypvt - yy_rhs_ct + 1);

      if ((yyr2[yyaction] & 1) == 1) { /* output associated code. */
        switch(yyaction) { 
          
          case 2:
{ yyerrok; } break;

          case 15:
{ yyerrok; } break;

          case 16:
{
                                          check_code_stack_size();
                                          common(LEA, DX, DIRECT, 0, ON);
                                          print_message();
                                          print_identifier(symbol_table[yypvt[0].location].length, symbol_table[yypvt[0].location].name_index);

                                          /*********************************************************************************
                                          ** Call getdec.                                                                 **
                                          *********************************************************************************/
                                          call_near_func(0x02);

                                          move_to_from_ax(symbol_table[yypvt[0].location].offset, OFF);
                                        } break;

          case 17:
{
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
                                         } break;

          case 18:
{
                                                   /*********************************************************************************
                                                   ** Subtract the two identifiers if there is a unary operator present.           **
                                                   ** Otherwise, add the two identifiers together.                                 **
                                                   *********************************************************************************/
                                                   if (yypvt[0].kind_of_location == -1) {
                                                      sub_mem_from_si(symbol_table[yypvt[0].location].offset);
                                                   }
                                                   else {
                                                      add_mem_to_si(symbol_table[yypvt[0].location].offset);
                                                   }
                                                 } break;

          case 19:
{
                                                   move_di_to_from_si(ON);
                                                   move_imm_to_si(yypvt[0].kind_of_location);
                                                   check_code_stack_size();
                                                   common(ADD, DI, SI, NO_DISPL, OFF);
                                                 } break;

          case 20:
{
                                                   /*********************************************************************************
                                                   ** Vice versa.                                                                  **
                                                   *********************************************************************************/
                                                   if (yypvt[0].kind_of_location == -1) {
                                                      add_mem_to_si(symbol_table[yypvt[0].location].offset);
                                                   }
                                                   else {
                                                      sub_mem_from_si(symbol_table[yypvt[0].location].offset);
                                                   }
                                                 } break;

          case 21:
{
                                                   move_di_to_from_si(ON);
                                                   move_imm_to_si(yypvt[0].kind_of_location);
                                                   check_code_stack_size();
                                                   common(SUB, DI, SI, NO_DISPL, ON);
                                                   move_di_to_from_si(OFF);
                                                 } break;

          case 22:
{
                                                   move_to_from_ax(symbol_table[yypvt[0].location].offset, ON);
                                                   if (yypvt[0].kind_of_location == -1) {
                                                      neg_si();
                                                   }
                                                   imul_si();
                                                   /*********************************************************************************
                                                   ** Note that the product cannot be greater than 16-bit.                         **
                                                   *********************************************************************************/
                                                   move_ax_to_from_si(OFF);
                                                 } break;

          case 23:
{
                                                   move_ax_to_from_si(ON);
                                                   move_imm_to_si(yypvt[0].kind_of_location);
                                                   imul_si();
                                                   /*********************************************************************************
                                                   ** Note that the product cannot be greater than 16-bit.                         **
                                                   *********************************************************************************/
                                                   move_ax_to_from_si(OFF);
                                                 } break;

          case 24:
{
                                                   move_ax_to_from_si(ON);
                                                   move_to_from_si(symbol_table[yypvt[0].location].offset, ON);
                                                   if (yypvt[0].kind_of_location == -1) {
                                                      neg_si();
                                                   }
                                                   idiv_si();
                                                   move_ax_to_from_si(OFF);
                                                 } break;

          case 25:
{
                                                   /*********************************************************************************
                                                   ** Check if denominator is not equal to zero. If it is, set                     **
                                                   ** div_by_zero_encountered flag to TRUE to prevent object module from being     **
                                                   ** produced. If it isn't, store the number in SI and then perform the division. **
                                                   *********************************************************************************/
                                                   if (yypvt[0].kind_of_location != 0) {
                                                      move_ax_to_from_si(ON);
                                                      move_imm_to_si(yypvt[0].kind_of_location);
                                                      idiv_si();
                                                      move_ax_to_from_si(OFF);
                                                   }
                                                   else {
                                                      div_by_zero_encountered = TRUE;
                                                   }
                                                 } break;

          case 26:
{
                                    move_to_from_si(symbol_table[yypvt[0].location].offset, ON);
                                    /*********************************************************************************
                                    ** If there is a unary minus in front of the identifier, negate it.             **
                                    *********************************************************************************/
                                    if (yypvt[0].kind_of_location == -1) {
                                       neg_si();
                                    }
                                  } break;

          case 27:
{
                                    is_string = TRUE;
                                  } break;

          case 29:
{
                                                   move_to_from_si(symbol_table[yypvt[-2].location].offset, OFF);
                                                 } break;

          case 30:
{
                                                   move_imm_to_si(yypvt[0].kind_of_location);
                                                   move_to_from_si(symbol_table[yypvt[-2].location].offset, OFF);
                                                 } break;

          case 31:
{
                                                          check_code_stack_size();
                                                          code_stack[code_stack_pos++] = 0xE9;
                                                          code_stack[code_stack_pos++] = yypvt[-2].kind_of_location  - (code_stack_pos + 2);
                                                          code_stack[code_stack_pos++] = (yypvt[-2].kind_of_location - (code_stack_pos + 2)) >> 8;
                                                          code_stack[yypvt[-2].location] = code_stack_pos - (yypvt[-2].location + 2);
                                                          code_stack[yypvt[-2].location + 1] = (code_stack_pos - (yypvt[-2].location + 2)) >> 8;
                                                        } break;

          case 32:
{
                                         yyval.kind_of_location = yypvt[-1].kind_of_location;
                                         yyval.location         = yypvt[0].location;
                                       } break;

          case 33:
{
                               yyval.kind_of_location = code_stack_pos;
                             } break;

          case 34:
{
                                                   /*********************************************************************************
                                                   ** If BX is not equal to CX, exit the loop.                                     **
                                                   *********************************************************************************/
                                                   compare_data(yypvt[-2].location, yypvt[0].location);
                                                   add_far_jump_instruction(0x85);
                                                   yyval.location = code_stack_pos - 2;
                                                 } break;

          case 35:
{
                                                   /*********************************************************************************
                                                   ** If BX is equal to CX, exit the loop.                                         **
                                                   *********************************************************************************/
                                                   compare_data(yypvt[-2].location, yypvt[0].location);
                                                   add_far_jump_instruction(0x84);
                                                   yyval.location = code_stack_pos - 2;
                                                 } break;

          case 36:
{
                                                   /*********************************************************************************
                                                   ** If BX is less than CX, exit the loop.                                        **
                                                   *********************************************************************************/
                                                   compare_data(yypvt[-2].location, yypvt[0].location);
                                                   add_far_jump_instruction(0x8C);
                                                   yyval.location = code_stack_pos - 2;
                                                 } break;

          case 37:
{
                                                   /*********************************************************************************
                                                   ** If BX is greater than CX, exit the loop.                                     **
                                                   *********************************************************************************/
                                                   compare_data(yypvt[-2].location, yypvt[0].location);
                                                   add_far_jump_instruction(0x8F);
                                                   yyval.location = code_stack_pos - 2;
                                                 } break;

          case 38:
{
                                                   /*********************************************************************************
                                                   ** If BX is less than or equal to CX, exit the loop.                            **
                                                   *********************************************************************************/
                                                   compare_data(yypvt[-2].location, yypvt[0].location);
                                                   add_far_jump_instruction(0x8E);
                                                   yyval.location = code_stack_pos - 2;
                                                 } break;

          case 39:
{
                                                   /*********************************************************************************
                                                   ** If BX is greater than or equal to CX, exit the loop.                         **
                                                   *********************************************************************************/
                                                   compare_data(yypvt[-2].location, yypvt[0].location);
                                                   add_far_jump_instruction(0x8D);
                                                   yyval.location = code_stack_pos - 2;
                                                 } break;

          case 40:
{
                                                        code_stack[yypvt[-2].location] = code_stack_pos - (yypvt[-2].location + 2);
                                                        code_stack[yypvt[-2].location + 1] = (code_stack_pos - (yypvt[-2].location + 2)) >> 8;
                                                      } break;

          case 41:
{
                                      yyval.location = yypvt[0].location;
                                    } break;

          case 42:
{
                                                      check_code_stack_size();
                                                      code_stack[code_stack_pos++] = 0xE9;
                                                      code_stack[code_stack_pos++] = yypvt[-3].location  - (code_stack_pos + 2);
                                                      code_stack[code_stack_pos++] = (yypvt[-3].location - (code_stack_pos + 2)) >> 8;
                                                      code_stack[yypvt[-3].kind_of_location] = code_stack_pos - (yypvt[-3].kind_of_location + 2);
                                                      code_stack[yypvt[-3].kind_of_location + 1] = (code_stack_pos - (yypvt[-3].kind_of_location + 2)) >> 8;
                                                    } break;

          case 43:
{
                                                 /*********************************************************************************
                                                 ** x <= val3                                                                    **
                                                 *********************************************************************************/
                                                 compare_data(identifier_index, yypvt[0].location);
                                                 add_far_jump_instruction(0x8F);
                                                 yyval.location = yypvt[-2].location;
                                                 yyval.kind_of_location = code_stack_pos - 2;
                                               } break;

          case 44:
{
                                                 /*********************************************************************************
                                                 ** x += val2                                                                    **
                                                 *********************************************************************************/
                                                 move_to_from_si(symbol_table[identifier_index].offset, ON);
                                                 add_mem_to_si(symbol_table[yypvt[0].location].offset);
                                                 move_to_from_si(symbol_table[identifier_index].offset, OFF);
                                                 code_stack[yypvt[-2].location] = code_stack_pos - (yypvt[-2].location + 2);
                                                 code_stack[yypvt[-2].location + 1] = (code_stack_pos - (yypvt[-2].location + 2)) >> 8;
                                                 yyval.location = yypvt[-2].location + 2;
                                               } break;

          case 45:
{
                                                 check_code_stack_size();
                                                 /*********************************************************************************
                                                 ** Generate a jump instruction to jump over the first increment. Note that      **
                                                 ** DO x = val1 STEP val2 TO val3 => for (x = val1; x <= val3; x += val2)        **
                                                 *********************************************************************************/
                                                 code_stack[code_stack_pos++] = 0xE9;
                                                 code_stack[code_stack_pos++] = 0x00;
                                                 code_stack[code_stack_pos++] = 0x00;
                                                 yyval.location = code_stack_pos - 2;
                                                 identifier_index = yypvt[0].location;
                                               } break;

        }
      } 

      yyps_pt -= yy_rhs_ct; /* pop yy_rhs_ct states from state stack. */
      yystate = * (yyps + yyps_pt -1); /* get current state. */
      YYGET_ACTION(yystate, yy_lhs, yyaction); /* get goto state. */
      if (yyaction == YYNOACTION) {
        YYERR_EXIT("yyparse symbol table error: goto state not found\n");
      }
      * (yyps + yyps_pt) = yyaction; /* push goto state onto stack. */
      if (++ yyps_pt == yystack_capacity) YYEXPAND_STACK; 

      /* push new value of $$ (yyval) onto value stack. */
      yypv_pt -= yy_rhs_ct;
      * (yypv + yypv_pt) = yyval;
      yypv_pt ++;

#if YYDEBUG
      yypm_pt -= yy_rhs_ct;
      * (yypm + yypm_pt) = yy_lhs;
      yypm_pt ++;

      fprintf(yyparse_fp, "after reduction: goto state=%d, lookahead=%s\n",
             * (yyps + yyps_pt - 1), yyget_tok(yychar));
#endif

    } else if (yyaction == 0) { /* is accept */
      if (yychar == YYEOF) {
#if YYDEBUG
        fprintf(yyparse_fp, "- valid accept\n");
#endif
        break; /* break out of while loop. */
      }
      else { /* this should not happen, since acc happens only on $. */
        yyerror("yyparse symbol table error: accept not on end marker");
#if YYDEBUG
        fprintf(yyparse_fp, "invalid accept. next lookahead is: %s\n",
                yyget_tok(yychar));
#endif
        YYABORT;
      } 
    }
  } /* end of while(1). */

#if YYDEBUG
  fclose(yyparse_fp);
  free(yypm);
#endif
  free(yyps);
  free(yypv);

  if (yyerr_hdl == -1) YYABORT;
  else YYACCEPT;
} /* end of yyparse */



