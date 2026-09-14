;Program 6.5
         BR      Main
i:       .BLOCK  d#2
lineFeed:.EQUATE h#000A
;
Main:    LOADA   d#0,i      ;for (i = 0
         STOREA  i,d
For:     COMPA   d#3,i      ;   i < 3
         BRGE    EndFor
         CHARO   c#/i/,i    ;cout << "i = " << i << endl
         CHARO   c#/ /,i
         CHARO   c#/=/,i
         CHARO   c#/ /,i
         DECO    i,d
         CHARO   lineFeed,i
         LOADA   i,d        ;   i++
         ADDA    d#1,i
         STOREA  i,d
         BR      For
EndFor:  CHARO   c#/i/,i    ;cout << "i = " << i << endl
         CHARO   c#/ /,i
         CHARO   c#/=/,i
         CHARO   c#/ /,i
         DECO    i,d
         CHARO   lineFeed,i
         STOP
         .END

