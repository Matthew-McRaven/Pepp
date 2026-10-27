BR      main
;******* getVect(int v[], int *n)
v:       .EQUATE 6           ;formal parameter #2h
n:       .EQUATE 4           ;formal parameter #2h
j:       .EQUATE 0           ;local variable #2d
getVect: SUBSP   2,i         ;push #j
@DECI   n,sf        ;scanf("%d", n)
LDWX    0,i         ;for (j = 0
STWX    j,s
for:     CPWX    n,sf        ;j < *n
BRGE    endFor
ASLX                ;two bytes per integer
@DECI   v,sfx       ;scanf("%d", &v[j])
LDWX    j,s         ;j++)
ADDX    1,i
STWX    j,s
BR      for
endFor:  ADDSP   2,i         ;pop #j
RET
;******* putVect(int v[], int n)
v2:      .EQUATE 6           ;formal parameter #2h
n2:      .EQUATE 4           ;formal parameter #2d
j2:      .EQUATE 0           ;local variable #2d
putVect: SUBSP   2,i         ;push #j2
LDWX    0,i         ;for (j = 0
STWX    j2,s
for2:    CPWX    n2,s        ;j < n
BRGE    endFor2
ASLX                ;two bytes per integer
@DECO   v2,sfx      ;printf("%d ", v[j])
@CHARO  ' ',i
LDWX    j2,s        ;j++)
ADDX    1,i
STWX    j2,s
BR      for2
endFor2: @CHARO  '\n',i      ;printf("\n")
ADDSP   2,i         ;pop #j2
RET
;******* main()
vector:  .EQUATE 2           ;local variable #2d6a
numItms: .EQUATE 0           ;local variable #2d
main:    SUBSP   14,i        ;push #vector #numItms
         MOVSPA              ;move (&)vector
         ADDA    vector,i
         STWA    -2,s
         MOVSPA              ;move &numItms
         ADDA    numItms,i
         STWA    -4,s
         SUBSP   4,i         ;push #v #n
         CALL    getVect     ;getVect(vector, &numItms)
         ADDSP   4,i         ;pop #n #v
         MOVSPA              ;move (&)vector
         ADDA    vector,i
         STWA    -2,s
         LDWA    numItms,s   ;move numItms
         STWA    -4,s
         SUBSP   4,i         ;push #v2 #n2
         CALL    putVect     ;putVect(vector, numItms)
         ADDSP   4,i         ;pop #n2 #v2
         ADDSP   14,i        ;pop #numItms #vector
         RET
