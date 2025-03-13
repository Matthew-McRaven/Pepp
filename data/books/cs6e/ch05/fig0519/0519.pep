;File: fig0519.pep
;Computer Systems, Fifth edition
;Figure 5.19
;
         LDWA    STRO,i
         SCALL   msg,d
         RET
msg:     .ASCII  "Hello, world!\n\0"
