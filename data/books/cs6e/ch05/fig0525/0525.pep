         @DECO   first,d     ;Interpret first as dec
         @CHARO  ' ',i
         @DECO   second,d    ;Interpret second and third as dec
         @CHARO  ' ',i
         @HEXO   second,d    ;Interpret second and third as hex
         @CHARO  ' ',i
         @CHARO  third,d     ;Interpret third as char
         @CHARO  fourth,d    ;Interpret fourth as char
         RET
first:   .WORD   0xFFFE
second:  .BYTE   0x00
third:   .BYTE   'U'
fourth:  .WORD   28673
