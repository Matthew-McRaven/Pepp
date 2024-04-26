;File: fig0510.pep
;Computer Systems, Fifth edition
;Figure 5.10
;
         LDBA    'H',i       ;Output 'H'
         STBA    0xFFFE, d
         LDBA    'i',i       ;Output 'i'
         STBA    0xFFFE,d
         STBA    0xFFFF,d    ;Store byte to power off port
