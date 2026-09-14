;Program 6.1
         BR      Main
number:  .BLOCK  d#2
;
Main:    DECI    number,d   ;cin >> number
If:      LOADA   number,d   ;if (number < 0)
         BRGE    EndIf
         LOADA   number,d   ;number = -number
         NOTA
         ADDA    d#1,i
         STOREA  number,d
EndIf:   DECO    number,d   ;cout << number
         STOP
         .END

