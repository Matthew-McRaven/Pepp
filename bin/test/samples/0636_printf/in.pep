;File: fig0636.pep
;Computer Systems, Fifth edition
;Figure 6.36
;
         BR      main
;
;******* main ()
vector:  .EQUATE 2           ;local variable #2d4a
j:       .EQUATE 0           ;local variable #2d
main:    SUBSP   10,i        ;push #vector #j
         LDWX    0,i         ;for (j = 0
         STWX    j,s
for1:    CPWX    4,i         ;j < 4
         BRGE    endFor1
         ASLX                ;two bytes per integer
         @DECI   vector,sx   ;scanf("%d", &vector[j])
         LDWX    j,s         ;j++)
         ADDX    1,i
         STWX    j,s
         BR      for1
endFor1: LDWX    3,i         ;for (j = 3
         STWX    j,s
for2:    CPWX    0,i         ;j >= 0
         BRLT    endFor2
         LDWA    msg,i
         STWA    -6,s
         LDWA    j,s
         STWA    -4,s
         ASLX                ;two bytes per integer
         LDWA    vector,sx
         STWA    -2,s
         SUBSP   6,i
         CALL    printf
         ADDSP   6,i
         LDWX    j,s         ;j--)
         SUBX    1,i
         STWX    j,s
         BR      for2
endFor2: ADDSP   10,i        ;pop #j #vector
         RET
msg:     .ASCII "%d %d\n\x00"
@LIBC
