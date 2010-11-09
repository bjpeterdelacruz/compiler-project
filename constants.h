#ifndef CONSTANTS
   #define CONSTANTS

   /* common.c */
   #define   AL            0x00
   #define   CL            0x01
   #define   DL            0x02
   #define   BL            0x03
   #define   AH            0x04
   #define   CH            0x05
   #define   DH            0x06
   #define   BH            0x07
   #define   AX            0x08
   #define   CX            0x09
   #define   DX            0x0A
   #define   BX            0x0B
   #define   SP            0x0C
   #define   BP            0x0D
   #define   SI            0x0E
   #define   DI            0x0F
   #define   DIRECT        0x10
   #define   INDIRECT_BX   0x11
   #define   INDIRECT_BP   0x12
   #define   ADD           0x02   /* including the direction bit, which is turned on by default */
   #define   OR            0x0A
   #define   ADC           0x12
   #define   SBB           0x1A
   #define   AND           0x22
   #define   SUB           0x2A
   #define   XOR           0x32
   #define   CMP           0x3A
   #define   MOV           0x8A
   #define   LEA           0x8D   /* LEA = 1000 11dw, where d = 0 and w = 1 */
   #define   DIV           0xF7   /* 16-bit */
   #define   IDIV          0xF7   /* 16-bit */
   #define   REG_FIELD        3
   #define   ON               1
   #define   OFF              0
   #define   NO_DISPL        -1

   /* everything else */
   #define   CODE_STACK_SIZE    25000
   #define   FIXUP_STACK_SIZE   10000
   #define   SIZE                5000
   #define   NUMBER_OF_LINES     1000
   #define   SIZES_STACK_SIZE     500
   #define   FILENAME_LENGTH      256
   #define   STRING_LENGTH        256
   #define   LINE_LENGTH          250
   #define   IDENTIFIER_LENGTH     25
   #define   NUMBER_OF_SPACES      11
   #define   CODE_SEGDEF_LENGTH    10
   #define   DATA_SEGDEF_LENGTH    10
   #define   STACK_SEGDEF_LENGTH   10
   #define   CARRIAGE_RTN_LINE_FD   2
   #define   OFFSET                 2
   #define   WORD_SIZE              2
   #define   CHECKSUM               1
   #define   SEGDEF_NUMBER          1
   #define   TRUE                   1
   #define   EXIT_SUCCESS           0
   #define   FALSE                  0
   #define   SYMREF                 0
   #define   EXIT_FAILURE          -1
#endif
