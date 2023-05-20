;File: fig0644.pep
;Computer Systems, Fifth edition
;Figure 6.44
;
         BR      main
;
;******* main()
a:       .EQUATE 4           ;local variable #2h
b:       .EQUATE 2           ;local variable #2h
c:       .EQUATE 0           ;local variable #2h
main:    SUBSP   6,i         ;push #a #b #c
         LDWA    2,i         ;a = (int *) malloc(sizeof(int))
         CALL    malloc      ;allocate #2d
         STWX    a,s
         LDWA    5,i         ;*a = 5
         STWA    a,sf
         LDWA    2,i         ;b = (int *) malloc(sizeof(int))
         CALL    malloc      ;allocate #2d
         STWX    b,s
         LDWA    3,i         ;*b = 3
         STWA    b,sf
         LDWA    a,s         ;c = a
         STWA    c,s
         LDWA    b,s         ;a = b
         STWA    a,s
         LDWA    2,i         ;*a = 2 + *c
         ADDA    c,sf
         STWA    a,sf
         LDWA    a,sf
         STWA    -6,s
         LDWA    b,sf
         STWA    -4,s
         LDWA    c,sf
         STWA    -2,s
         LDWA    msg,i
         STWA    -8,s
         SUBSP   8,i
         CALL    printf
         ADDSP   8,i
         ADDSP   6,i         ;pop #c #b #a
         RET
msg:     .ASCII  "*a = %d\n*b = %d\n*c = %d\n\x00"
;
         @LIBC
         @MALLOC
