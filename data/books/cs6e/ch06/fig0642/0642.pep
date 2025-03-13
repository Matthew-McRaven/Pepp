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
         @STRO   msg0,d      ;printf("*a = %d\n", *a)
         @DECO   a,n
         @CHARO  '\n',i
         @STRO   msg1,d      ;printf("*b = %d\n", *b)
         @DECO   b,n
         @CHARO  '\n',i
         @STRO   msg2,d      ;printf("*c = %d\n", *c)
         @DECO   c,n
         @CHARO  '\n',i
         RET
msg0:    .ASCII  "*a = \0"
msg1:    .ASCII  "*b = \0"
msg2:    .ASCII  "*c = \0"
         @MALLOC
