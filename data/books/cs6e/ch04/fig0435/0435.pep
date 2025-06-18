LDBA    0xFFFD,d    ;Load byte first char from input port
STBA    0x0015,d    ;Store byte first char to 0015
LDBA    0xFFFD,d    ;Load byte from input port
STBA    0xFFFE,d    ;Store byte to output port
LDBA    0x0015,d    ;Load byte first char from 0015
STBA    0xFFFE,d    ;Store byte first char to output port
STBA    0xFFFF,d    ;Store byte to power off port
.BLOCK  1           ;One byte storage for first char
