BR      main
;
;******* void swap(int *r, int *s)
r:       .EQUATE 6           ;formal parameter #2h
s:       .EQUATE 4           ;formal parameter #2h
temp:    .EQUATE 0           ;local variable #2d
swap:    SUBSP   2,i         ;push #temp
LDWA    r,sf        ;temp = *r
STWA    temp,s
LDWA    s,sf        ;*r = *s
STWA    r,sf
LDWA    temp,s      ;*s = temp
STWA    s,sf
ADDSP   2,i         ;pop #temp
RET
;
;******* void order(int *x, int *y)
x:       .EQUATE 4           ;formal parameter #2h
y:       .EQUATE 2           ;formal parameter #2h
order:   LDWA    x,sf        ;if (*x > *y)
CPWA    y,sf
BRLE    endIf
LDWA    x,s         ;move x
STWA    -2,s
LDWA    y,s         ;move y
STWA    -4,s
SUBSP   4,i         ;push #r #s
CALL    swap        ;swap(x, y)
ADDSP   4,i         ;pop #s #r
endIf:   RET
;
;******* main()
a:       .EQUATE 2           ;local variable #2d
b:       .EQUATE 0           ;local variable #2d
main:    SUBSP   4,i         ;push #a #b
         @STRO   msg1,d      ;printf("Enter an integer: ")
         @DECI   a,s         ;scanf("%d", &a)
         @STRO   msg1,d      ;printf("Enter an integer: ")
         @DECI   b,s         ;scanf("%d", &b)
         MOVSPA              ;move &a
         ADDA    a,i
         STWA    -2,s
         MOVSPA              ;move &b
         ADDA    b,i
         STWA    -4,s
         SUBSP   4,i         ;push #x #y
         CALL    order       ;order(&a, &b)
ra1:     ADDSP   4,i         ;pop #y #x
         @STRO   msg2,d      ;printf("Ordered they are: %d, %d\n"
         @DECO   a,s         ;, a
         @STRO   msg3,d
         @DECO   b,s         ;, b)
         @CHARO  '\n',i
         ADDSP   4,i         ;pop #b #a
         RET
msg1:    .ASCII  "Enter an integer: \0"
msg2:    .ASCII  "Ordered they are: \0"
msg3:    .ASCII  ", \0"
