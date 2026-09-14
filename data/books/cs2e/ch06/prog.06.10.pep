;Program 6.10
         BR      Main
;
;------- int BinCoeff (int n, int k)
retVal:  .EQUATE d#10       ;returned value
n:       .EQUATE d#8        ;formal parameter
k:       .EQUATE d#6        ;formal parameter
y1:      .EQUATE d#2        ;local variable
y2:      .EQUATE d#0        ;local variable
BinCoeff:ADDSP   d#-4,i     ;allocate locals
If:      LOADA   k,s        ;if ((k == 0)
         BREQ    Then
         LOADA   n,s        ;|| (n == k))
         COMPA   k,s
         BRNE    Else
Then:    LOADA   d#1,i      ;return 1
         STOREA  retVal,s
         BR      EndIf
Else:    LOADA   n,s        ;push n - 1
         SUBA    d#1,i
         STOREA  d#-4,s
         LOADA   k,s        ;push k
         STOREA  d#-6,s
         ADDSP   d#-6,i     ;push params and retVal
         JSR     BinCoeff   ;BinomCoeff(n - 1, k)
         ADDSP   d#6,i      ;pop params and retVal
         LOADA   d#-2,s     ;y1 = BinomCoeff(n - 1, k)
         STOREA  y1,s
         LOADA   n,s        ;push n - 1
         SUBA    d#1,i
         STOREA  d#-4,s
         LOADA   k,s        ;push k - 1
         SUBA    d#1,i
         STOREA  d#-6,s
         ADDSP   d#-6,i     ;push params and retVal
         JSR     BinCoeff   ;BinomCoeff(n - 1, k - 1)
         ADDSP   d#6,i      ;pop params and retVal
         LOADA   d#-2,s     ;y2 = BinomCoeff(n - 1, k - 1)
         STOREA  y2,s
         LOADA   y1,s       ;return y1 + y2
         ADDA    y2,s
         STOREA  retVal,s
EndIf:   ADDSP   d#4,i      ;pop locals
         RTS
;
;------- main()
Main:    LOADA   d#3,i      ;push 3
         STOREA  d#-4,s
         LOADA   d#1,i      ;push 1
         STOREA  d#-6,s
         ADDSP   d#-6,i     ;push params and retVal
         JSR     BinCoeff   ;BinomCoeff(3, 1)
         ADDSP   d#6,i      ;pop params and retVal
         DECO    d#-2,s     ;output retVal
         STOP
         .END

