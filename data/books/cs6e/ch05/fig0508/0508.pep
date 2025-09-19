         @CHARI  char_1,d    ;Input first char to char_1
         LDBA    charIn,d    ;Load from input port
         STBA    charOut,d   ;Store to output port
         @CHARO  char_1,d    ;Output char_1
         STBA    pwrOff,d    ;Store to power off port
char_1: .BLOCK   1           ;One byte storage for first char
