/**************************************************
**                                               **
** BJ Peter DeLaCruz                             **
** October , 2010                                **
** ICS 611                                       **
**                                               **
** Project #4                                    **
**                                               **
**                                               **
**                                               **
**************************************************/

#include "common.h"
#include "constants.h"

unsigned char code_stack[CODE_STACK_SIZE];
unsigned char fixup_stack[FIXUP_STACK_SIZE];
int code_stack_pos  = 0;
int fixup_stack_pos = 0;
extern int prev_code_stack_pos;

int common(int opcode_operand, int reg_operand, int other_operand, int displ, int direction) {

    unsigned char byte = 0x00;
 
    /* It is assumed that the direction bit in opcode_operand is turned on. */
    byte |= opcode_operand;
    if (reg_operand >= AX && reg_operand <= DI && opcode_operand != LEA) {
       byte++;
    }
    if (direction == OFF) {
       byte ^= 0x02;
    }
    add_to_code_stack(byte);

    /* e.g. SUB BL, CL */
    if (displ == NO_DISPL) {
       byte = 0xC0; /* mm = 11 */
       byte |= (reg_operand << REG_FIELD);
       byte |= other_operand; /* r/m field = second register */
       if (reg_operand >= AX && reg_operand <= BX) {
          byte ^= 0x08;
       }
       add_to_code_stack(byte);
       return 0;
    }

    /* e.g. SUB BX, [BX + 0005H] */
    if (other_operand == INDIRECT_BX || other_operand == INDIRECT_BP) {
       if (displ > 0xFF) { /* 16-bit displacement */
          byte = 0x80;
       }
       else if (displ == 0x00) { /* no displacement */
          byte = 0x00;
       }
       else { /* 8-bit displacement */
          byte = 0x40;
       }
       byte |= (reg_operand << REG_FIELD);
       /* Set mm = 10 if the displacement is 16-bit. */
       if (displ > 0xFF) {
          byte ^= 0x40;
       }
       if (other_operand == INDIRECT_BX) {
          byte |= 0x07;
       }
       else {
          byte |= 0x06;
       }
    }
    /* e.g. SUB BX, [2A43H] */
    else {
       byte = 0x06; /* r/m = 110 */
       byte |= (reg_operand << REG_FIELD);
       /* Set mm = 00 if register is 16-bit. */
       if (reg_operand >= AX && reg_operand <= DI) {
          byte ^= 0x40;
       }
    }

    add_to_code_stack(byte);

    /* Add displacement to the code stack. */
    if (displ > 0xFF) { /* 16-bit displacement */
       /* Store low-order byte first, e.g. 05H. */
       byte = displ;
       add_to_code_stack(byte);

       byte = displ >> 8; /* Then store high-order byte, e.g. 00H. */
       add_to_code_stack(byte);
    }
    else { /* 8-bit displacement */
       byte = displ;
       add_to_code_stack(byte);
       byte = 0x00;
       add_to_code_stack(byte);
    }

    if (other_operand == DIRECT) {
       fix_offset();
    }

    return 0;

}

void fix_offset() {
    int size = code_stack_pos - prev_code_stack_pos - 2;

    if (size < 0x0100) {
       fixup_stack[fixup_stack_pos++] = 0xC4;
    }
    else if (size >= 0x0100 && size < 0x0200) {
       fixup_stack[fixup_stack_pos++] = 0xC5;
    }
    else if (size >= 0x0200 && size < 0x0300) {
       fixup_stack[fixup_stack_pos++] = 0xC6;
    }
    else if (size >= 0x0300) {
       fixup_stack[fixup_stack_pos++] = 0xC7;
    }
    fixup_stack[fixup_stack_pos++] = size;
    fixup_stack[fixup_stack_pos++] = 0x54;
    fixup_stack[fixup_stack_pos++] = 0x02; /* assumes 2nd segdef */
}

void add_to_code_stack(char code) {
    code_stack[code_stack_pos++] = code;
}

void print_character_in_binary(char character) {

    int i;
    for (i = 7; i >= 0; i--) {
        if ((1 << i) & character) {
           printf("1");
        }
        else {
           printf("0");
        }
    }

}

void print_code_stack() {

    int i, count;
    for (i = 0, count = 1; i < code_stack_pos; i++, count++) {
        print_character_in_binary(code_stack[i]);
        printf(" ");
        if (count % 7 == 0) {
           printf("\n");
        }
    }

}

void print_code_stack_in_hex() {

    int i, count;
    for (i = 0, count = 1; i < code_stack_pos; i++, count++) {
        if (code_stack[i] < 0x10) { printf("0"); }
        printf("%X ", code_stack[i]);
        if (count % 7 == 0) {
           printf("\n");
        }
    }

}
