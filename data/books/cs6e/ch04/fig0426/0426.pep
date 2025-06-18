LDWA  0x000f,d  ;Load word accumulator 0005 from Mem[000F]
ADDA  0x0011,d  ;Add accumulator 0003 from Mem[0011]
ORA   0x0013,d  ;Or accumulator 0030 from Mem[0013]
STBA  0xFFFE,d  ;Store byte to output port
STBA  0xFFFF,d  ;Store byte to power off port
.WORD 5         ;Decimal 5
.WORD 3         ;Decimal 3
.WORD 0x30      ;Mask for ASCII char
