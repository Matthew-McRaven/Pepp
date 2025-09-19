         LDBA    charIn,d    ;Input first digit
         STWA    num,d       ;Store to num
         LDBA    charIn,d    ;Input second digit
         ADDA    num,d       ;Add num to second digit
         ANDA    andMask,d   ;Zero out the left nybble
         ORA     orMask,d    ;Convert sum to ASCII character
         STBA    charOut,d   ;Output sum
         STBA    pwrOff,d    ;Shut down
num:     .BLOCK  2
andMask: .WORD   0x000F
orMask:  .WORD   0x0030
