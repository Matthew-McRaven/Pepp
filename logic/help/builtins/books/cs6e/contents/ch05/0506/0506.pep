;File: fig0506.pep
;Computer Systems, Fifth edition
;Figure 5.6
;
         LDBA    0xFFFD,d    ;Input first character
         STBA    0x0015,d    ;Store first character
         LDBA    0xFFFD,d    ;Input second character
         STBA    0xFFFE,d    ;Output second character
         LDBA    0x0015,d    ;Load first character
         STBA    0xFFFE,d    ;Output first character
         STBA    0xFFFF,d    ;Store byte to power off port
         .BLOCK  1           ;Storage for first character
