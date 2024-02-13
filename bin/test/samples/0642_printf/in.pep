;File: fig0642.pep
;Computer Systems, Fifth edition
;Figure 6.42
;
         BR      main
a:       .BLOCK  2           ;global variable #2h
b:       .BLOCK  2           ;global variable #2h
c:       .BLOCK  2           ;global variable #2h
;
;******* main ()
main:    LDWA    2,i         ;a = (int *) malloc(sizeof(int))
         CALL    malloc      ;allocate #2d
         STWX    a,d
         LDWA    5,i         ;*a = 5
         STWA    a,n
         LDWA    2,i         ;b = (int *) malloc(sizeof(int))
         CALL    malloc      ;allocate #2d
         STWX    b,d
         LDWA    3,i         ;*b = 3
         STWA    b,n
         LDWA    a,d         ;c = a
         STWA    c,d
         LDWA    b,d         ;a = b
         STWA    a,d
         LDWA    2,i         ;*a = 2 + *c
         ADDA    c,n
         STWA    a,n
         LDWA    a,n
         STWA    -6,s
         LDWA    b,n
         STWA    -4,s
         LDWA    c,n
         STWA    -2,s
         LDWA    msg,i
         STWA    -8,s
         SUBSP   8,i
         call    printf
         ADDSP   8,i
         RET
msg:     .ASCII  "*a = %d\n*b = %d\n*c = %d\n\x00"
         @LIBC
         @MALLOC
