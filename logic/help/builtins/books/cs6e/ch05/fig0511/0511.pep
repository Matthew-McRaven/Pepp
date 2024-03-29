;File: fig0511.pep
;Computer Systems, Fifth edition
;Figure 5.11
;
         BR      0x0005      ;Branch around data
         .BLOCK  2           ;Storage for one integer
;
         LDWA    DECI,i      ;Get the number
         SCALL   0x0003,d
         LDWA    DECO,i      ;and output it
         SCALL   0x0003,d
         LDBA    ' ',i       ;Output " + 1 = "
         STBA    0xFFFE,d
         LDBA    '+',i
         STBA    0xFFFE,d
         LDBA    ' ',i
         STBA    0xFFFE,d
         LDBA    '1',i
         STBA    0xFFFE,d
         LDBA    ' ',i
         STBA    0xFFFE,d
         LDBA    '=',i
         STBA    0xFFFE,d
         LDBA    ' ',i
         STBA    0xFFFE,d
         LDWA    0x0003,d    ;A <- the number
         ADDA    1,i         ;Add one to it
         STWA    0x0003,d    ;Store the sum
         LDWA    DECO,i      ;Output the sum
         SCALL   0x0003,d
         RET
