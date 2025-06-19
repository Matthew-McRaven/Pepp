    LDBA 0xFFFD,d   ;Load byte accumulator from input port
    STWA b1s,d      ;Store word accumulator to Mem[0018]
    LDBA 0xFFFD,d   ;Load byte accumulator from input port
ds: ADDA b1s,d      ;Add accumulator from Mem[0018]
    ANDA  am,d      ;And accumulator from Mem[001A]
    ORA om,d        ;Or accumulator from Mem[001C]
    STBA 0xFFFE,d   ;Store byte to output port
    STBA 0xFFFF,d   ;Store byte to power off port
b1s:.WORD 0         ;One-word storage for first number
am: .WORD 0x0f      ;AND mask
om: .WORD 0x30      ;OR mask

