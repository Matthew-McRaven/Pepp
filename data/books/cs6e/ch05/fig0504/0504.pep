         LDBA    0xFFFD,d    ;Load first char from input port
         STBA    0x0015,d    ;Store first char to 0015
         LDBA    0xFFFD,d    ;Load from input port
         STBA    0xFFFE,d    ;Store to output port
         LDBA    0x0015,d    ;Load first char from 0015
         STBA    0xFFFE,d    ;Store first char to output port
         STBA    0xFFFF,d    ;Store to power off port
         .BLOCK  1           ;One byte storage for first char
