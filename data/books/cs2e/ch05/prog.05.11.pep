;Program 5.11
         BR      Main
ch:      .BLOCK  d#1
i:       .BLOCK  d#2
lineFeed:.EQUATE h#000A
;
Main:    CHARI   ch,d       ;cin >> ch >> i
         DECI    i,d
         LOADA   i,d        ;i += 5
         ADDA    d#5,i
         STOREA  i,d
         LDBYTA  ch,d       ;ch++
         ADDA    d#1,i
         STBYTA  ch,d
         CHARO   ch,d       ;cout << ch << endl << i
         CHARO   lineFeed,i
         DECO    i,d
         STOP
         .END
