;Program 6.3
         BR      Main
letter:  .BLOCK  d#1
;
Main:    CHARI   letter,d   ;cin >> letter
         LOADA   h#0000,i
While:   LDBYTA  letter,d   ;while (letter != '*')
         COMPA   c#/*/,i
         BREQ    EndWh
         CHARO   letter,d   ;cout << letter
         CHARI   letter,d   ;cin >> letter
         BR      While
EndWh:   STOP
         .END

