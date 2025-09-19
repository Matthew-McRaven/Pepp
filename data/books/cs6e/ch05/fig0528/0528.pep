         LDBA    0x81,i
         STBA    there,d
         LDWA    num_1,d
there:   SUBA    num_2,d
         STWA    num_3,d
         @DECO   num_3,d
         RET
num_1:   .WORD   243
num_2:   .WORD   7
num_3:   .BLOCK  2
