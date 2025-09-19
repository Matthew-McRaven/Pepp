LDBA 0x000F,d    ;Load byte accumulator 'H'
STBA 0xFFFE,d    ;Store byte accumulator output port
ldba    0x0010  ,  D  ;Load byte accumulator 'i'
STBA 0xfffe,d    ;Store byte accumulator output port
STBA 0xFFFF,d    ;Store byte power off port
.ASCII "Hi"      ;ASCII "Hi" characters

