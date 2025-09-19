         LDBA    charIn,d    ;Load first char from input port
         STBA    char_1,d    ;Store first char to char_1
         LDBA    charIn,d    ;Load from input port
         STBA    charOut,d   ;Store to output port
         LDBA    char_1,d    ;Load first char from char_1
         STBA    charOut,d   ;Store first char to output port
         STBA    pwrOff,d    ;Store to power off port
char_1:  .BLOCK  1           ;One byte storage for first char
