;File: fig0507.pep
;Computer Systems, Fifth edition
;Figure 5.7
;
         LDWA    0x000F,d    ;A <- first number
         ADDA    0x0011,d    ;Add the two numbers
         ORA     0x0013,d    ;Convert sum to character
         STBA    0xFAAC,d    ;Output the character
         STBA    0xFAAE,d    ;Store byte to power off port
         .WORD   5           ;Decimal 5
         .WORD   3           ;Decimal 3
         .WORD   0x0030      ;Mask for ASCII char
         .END
