         BR      main
;
LOWER:   .EQUATE 21          ;const int
UPPER:   .EQUATE 65          ;const int
;
;******* bool inRange(int a)
retVal:  .EQUATE 4           ;returned value #2d
a:       .EQUATE 2           ;formal parameter #2d
inRange: LDWA    LOWER,i     ;if ((LOWER <= a)
if:      CPWA    a,s
         BRGT    endif
         LDWA    a,s         ;&& (a <= UPPER))
         CPWA    UPPER,i
         BRGT    endif
then:    LDWA    true,i      ;return true
         STWA    retVal,s
         RET
endif:   LDWA    false,i     ;return false
         STWA    retVal,s
         RET
;
;******* main()
age:     .EQUATE 0           ;local variable #2d
main:    SUBSP   2,i         ;push #age
         @DECI   age, s      ;scanf("%d", &age)
         LDWA    age,s       ;move age
         STWA    -4,s
         SUBSP   4,i         ;push #retVal #a
         CALL    inRange     ;inRange(age)
         ADDSP   4,i         ;pop #a #retVal
         LDWA    -2,s        ;if (inRange(age))
         BREQ    else2
then2:   @STRO   msg1, d     ;printf("Qualified\n")
         BR      endif2
else2:   @STRO   msg2, d     ;printf("Unqualified\n");
endif2:  ADDSP   2,i         ;pop #age
         RET
msg1:    .ASCII  "Qualified\n\0"
msg2:    .ASCII  "Unqualified\n\0"

