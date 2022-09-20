;File: fig0510.pep
;Computer Systems, Fifth edition
;Figure 5.10
;
         LDBA    'H',i       ;Output 'H'
         STBA    0xFAAC,d
         LDBA    'i',i       ;Output 'i'
         STBA    0xFAAC,d
         STBA    0xFAAE,d    ;Store byte to power off port
         .END
