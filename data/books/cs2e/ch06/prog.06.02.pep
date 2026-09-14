;Program 6.2
         BR      Main
limit:   .EQUATE d#100
num:     .BLOCK  d#2
;
Main:    DECI    num,d      ;cin >> num
If:      LOADA   num,d      ;if (num >= limit)
         COMPA   limit,i
         BRLT    Else
         CHARO   c#/h/,i    ;cout << "high"
         CHARO   c#/i/,i
         CHARO   c#/g/,i
         CHARO   c#/h/,i
         BR      EndIf      ;else
Else:    CHARO   c#/l/,i    ;cout << "low"
         CHARO   c#/o/,i
         CHARO   c#/w/,i
EndIf:   STOP
         .END

