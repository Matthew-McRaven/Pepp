;File: fig0506.pep
;Computer Systems, Fifth edition
;Figure 5.6
;
         LDBA    0xFAAA,d    ;Input first character
         STBA    0x0015,d    ;Store first character
         LDBA    0xFAAA,d    ;Input second character
         STBA    0xFAAC,d    ;Output second character
         LDBA    0x0015,d    ;Load first character
         STBA    0xFAAC,d    ;Output first character
         STBA    0xFAAE,d    ;Store byte to power off port
         .BLOCK  1           ;Storage for first character
         .END
