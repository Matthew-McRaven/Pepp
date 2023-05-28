;File: fig0832.pep
;Computer Systems, Fifth edition
;Figure 08.34
;
         BR      main        ;branch around data
num:     .BLOCK  2           ;global variable
main:    DECI    num,d       ;input decimal value
         DECO    num,d       ;output decimal value
         LDBA    '\n',i
         STBA    charOut,d
         STRO    msg,d       ;output message
         STOP
msg:     .ASCII  "That's all.\n\x00"
         .END
