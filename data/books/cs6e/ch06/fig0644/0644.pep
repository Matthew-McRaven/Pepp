         BR      main
vector:  .BLOCK  8           ;global variable #2d4a
j:       .BLOCK  2           ;global variable #2d
;
;******* main()
main:    LDWX    0,i         ;for (j = 0
         STWX    j,d
for1:    LDWX    j,d         ;j < 4
         CPWX    4,i
         BRGE    endFor1
         LDWX    j,d         ;scanf("%d", &vector[j])
         ASLX                ;two bytes per integer
         @DECI   vector, x
         LDWX    j,d         ;j++)
         ADDX    1,i
         STWX    j,d
         BR      for1
endFor1: LDWX    3,i         ;for (j = 3
         STWX    j,d
for2:    LDWX    j,d         ;j >= 0
         CPWX    0,i
         BRLT    endFor2
         LDWX    j,d         ;printf("%d %d\n", j, vector[j])
         @DECO   j, d
         @CHARO  ' ', i
         LDWX    j,d
         ASLX                ;two bytes per integer
         @DECO   vector, x
         @CHARO  '\n', i
         LDWX    j,d         ;j--)
         SUBX    1,i
         STWX    j,d
         BR      for2
endFor2: RET

