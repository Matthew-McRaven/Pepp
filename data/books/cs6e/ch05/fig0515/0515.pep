;File: fig0515.pep
;Computer Systems, Fifth edition
;Figure 5.15
;
         BR      main        ;Branch around data
num:     .BLOCK  2           ;Storage for one integer #2d
;
main:    LDWA    DECI,i      ;Get the number
         SCALL   num,d
         LDWA    DECO,i      ;and output it
         SCALL   num,d
         LDWA    STRO,i      ;Output " + 1 = "
         SCALL   msg,d
         LDWA    num,d       ;A <- the number
         ADDA    1,i         ;Add one to it
         STWA    num,d       ;Store the sum
         LDWA    DECO,i      ;Output the sum
         SCALL   num,d
         RET
msg:     .ASCII  " + 1 = \0"
