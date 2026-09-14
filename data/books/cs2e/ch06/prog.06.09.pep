;Program 6.9
         BR      Main
numPts:  .BLOCK  d#2
value:   .BLOCK  d#2
i:       .BLOCK  d#2
lineFeed:.EQUATE h#000A
;
;------- void PrintBar (int n)
n:       .EQUATE d#4        ;formal parameter
j:       .EQUATE d#0        ;local variable
PrintBar:ADDSP   d#-2,i     ;allocate local
         LOADA   d#1,i      ;initialize j = 1
         STOREA  j,s
For1:    COMPA   n,s        ;test for j <= n
         BRGT    EndFor1
         CHARO   c#/*/,i    ;cout << '*'
         LOADA   j,s        ;increment j++
         ADDA    d#1,i
         STOREA  j,s
         BR      For1
EndFor1: CHARO   lineFeed,i ;cout << endl
         ADDSP   d#2,i      ;deallocate local
         RTS                ;pop RetAddr
;
;------- main()
Main:    DECI    numPts,d   ;cin >> numPts
         LOADA   d#1,i      ;initialize i = 1
         STOREA  i,d
For2:    COMPA   numPts,d   ;test for i <= numPts
         BRGT    EndFor2
         DECI    value,d    ;cin >> value
         LOADA   value,d    ;call by value
         STOREA  d#-2,s
         ADDSP   d#-2,i     ;push parameter
         JSR     PrintBar   ;push RetAddr
         ADDSP   d#2,i      ;pop parameter
         LOADA   i,d        ;increment i++
         ADDA    d#1,i
         STOREA  i,d
         BR      For2
EndFor2: STOP
         .END

