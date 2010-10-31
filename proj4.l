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
    #include "y.tab.h"

    typedef struct {
        int kind_of_location;
        int location;
    } yystype;
    #define   YYSTYPE   yystype

    extern YYSTYPE yylval;

    typedef struct {
        int name_index;
        int offset;
        int hash_link;
        int length;
    } symbol_table_entry;

    char id_stack[SIZE];

    int data_segment_offset = 0;
    int id_stack_index      = 0;
    int num_bytes           = 0;
    int symbol_table_index  = 127;

    symbol_table_entry symbol_table[SIZE];

    int column_number = 0;
    int position      = 0;
    int word_length   = 0;

    char string[STRING_LENGTH];
%}

main        "MAIN"
end         "END"
int         "INT"
input       "INPUT"
output      "OUTPUT"
while       "WHILE"
wend        "WEND"
if          "IF"
endif       "ENDIF"
do          "DO"
step        "STEP"
to          "TO"
eq          "=="
ne          "!="
gte         ">="
lte         "<="
gt          ">"
lt          "<"
alpha       [a-zA-Z0-9]
digit       [0-9]
punct       [~`!@#$%^&*()_+={}\[\]|\\:;'<>,.?/-]
whitespace  " "

%x c_comment
%x cpp_comment
%option noyywrap
%%

("/*")              { BEGIN(c_comment); }
<c_comment>\n       { word_length = strlen(yytext); column_number = 0; yyerror("eol"); }
<c_comment>.        ;
<c_comment>("*/")   { BEGIN(0); }

"//"                { BEGIN(cpp_comment); }
<cpp_comment>.      ;
<cpp_comment>\n     { word_length = strlen(yytext); column_number = 0; yyerror("eol"); BEGIN(0); }

{main}          { get_word_length_and_column_number(strlen(yytext), yyleng); return MAIN;      }

{end}           { get_word_length_and_column_number(strlen(yytext), yyleng); return END;       }

{int}           { get_word_length_and_column_number(strlen(yytext), yyleng); return INT;       }

{input}         { get_word_length_and_column_number(strlen(yytext), yyleng); return INPUT;     }

{output}        { get_word_length_and_column_number(strlen(yytext), yyleng); return OUTPUT;    }

{wend}          { get_word_length_and_column_number(strlen(yytext), yyleng); return WEND;      }

{while}         { get_word_length_and_column_number(strlen(yytext), yyleng); return WHILE;     }

{eq}            { get_word_length_and_column_number(strlen(yytext), yyleng); return EQ;        }

{ne}            { get_word_length_and_column_number(strlen(yytext), yyleng); return NE;        }

{gte}           { get_word_length_and_column_number(strlen(yytext), yyleng); return GTE;       }

{lte}           { get_word_length_and_column_number(strlen(yytext), yyleng); return LTE;       }

{gt}            { get_word_length_and_column_number(strlen(yytext), yyleng); return GT;        }

{lt}            { get_word_length_and_column_number(strlen(yytext), yyleng); return LT;        }

{if}            { get_word_length_and_column_number(strlen(yytext), yyleng); return IF;        }

{endif}         { get_word_length_and_column_number(strlen(yytext), yyleng); return ENDIF;     }

{do}            { get_word_length_and_column_number(strlen(yytext), yyleng); return DO;        }

{step}          { get_word_length_and_column_number(strlen(yytext), yyleng); return STEP;      }

{to}            { get_word_length_and_column_number(strlen(yytext), yyleng); return TO;        }

({digit})+      { get_word_length_and_column_number(strlen(yytext), yyleng); yylval.kind_of_location = atoi(yytext); return NUMBER; }

"-"({digit})+   { get_word_length_and_column_number(strlen(yytext), yyleng); yylval.kind_of_location = atoi(yytext); return NUMBER; }

({alpha})+      {
                  get_word_length_and_column_number(strlen(yytext), yyleng);
                  yylval.kind_of_location = SYMREF;
                  yylval.location         = find(yytext);
                  num_bytes++;
                  return IDENTIFIER;
                }

"-"({alpha})+   {
                  char* id       = (char*) malloc(sizeof(char) * strlen(yytext));
                  char* pcString = yytext + 1;

                  strcpy(id, pcString);
                  get_word_length_and_column_number(strlen(yytext), yyleng);
                  yylval.kind_of_location = -1;
                  yylval.location         = find(id);
                  num_bytes++;
                  free(id);
                  return IDENTIFIER;
                }

"\""({alpha}|{whitespace}|{punct})+"\"" {
                                          char* pcString = yytext + 1;

                                          get_word_length_and_column_number(strlen(yytext), yyleng);
                                          strcpy(string, pcString);
                                          pcString  = strstr(string, "\"");
                                          *pcString = '\0';
                                          return STRING;
                                        }

({whitespace})* {
                  /*********************************************************************************
                  ** Assume that the source code file does not contain any tabs.                  **
                  *********************************************************************************/
                  get_word_length_and_column_number(strlen(yytext), yyleng);
                }

\n              {
                  word_length   = strlen(yytext);
                  column_number = 0;
                  yyerror("eol");
                }

.               { get_word_length_and_column_number(strlen(yytext), yyleng); return yytext[0]; }

%%

/***************************************************
************** Symbol table functions **************
***************************************************/

int find(char* name) {
    int j = hash(name);

    if (symbol_table[j].name_index == -1) {
       make_entry(name, j);
       return j;
    }
    else if (cmpstr(id_stack + symbol_table[j].name_index, name)) {
       return j;
    }

    while (symbol_table[j].hash_link != -1) {
       if (cmpstr(id_stack + symbol_table[j].name_index, name)) {
          return j;
       }
       j = symbol_table[j].hash_link;
    }
   
    symbol_table[j].hash_link = symbol_table_index;
    make_entry(name, symbol_table_index);
    ++symbol_table_index;
    return symbol_table_index - 1;
}

int hash(char* name) {
    int hash_value = 0;
    int i = 0;

    while (name[i] != '\0') {
       hash_value += name[i++];
    }
    return(hash_value % 127);
}

void make_entry(char* name, int j) {
    symbol_table[j].name_index = id_stack_index;
    symbol_table[j].offset = data_segment_offset + CARRIAGE_RTN_LINE_FD + strlen(input_msg) + CARRIAGE_RTN_LINE_FD + strlen(output_msg);
    symbol_table[j].length = strlen(name);
    data_segment_offset += 2;
    strcat(id_stack, name);
    id_stack_index += strlen(name);
}

void get_word_length_and_column_number(int yytext_length, int yyleng_value) {
    word_length = yytext_length;
    for (position = 0; position < yyleng_value; position++, column_number++);
}
