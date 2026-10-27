BR      main
age:     .BLOCK  2           ;global variable #2d
;
main:    @DECI   age,d       ;scanf("%d", &age)
@STRO   msg1,d      ;printf("Age: %d years\n", age)
@DECO   age,d
@STRO   msg2,d
RET                 ;return 0
msg1:    .ASCII  "Age: \0"
msg2:    .ASCII  " years\n\0"
