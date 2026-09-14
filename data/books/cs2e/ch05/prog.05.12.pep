;Program 5.12
         BR      Main
bonus:   .EQUATE d#5
exam1:   .BLOCK  d#2
exam2:   .BLOCK  d#2
score:   .BLOCK  d#2
;
Main:    DECI    exam1,d    ;cin >> exam1 >> exam2
         DECI    exam2,d    ;   >> exam2
         LOADA   exam1,d    ;score = (exam1
         ADDA    exam2,d    ;   + exam2)
         ASRA               ;   / 2
         ADDA    bonus,i    ;   + bonus
         STOREA  score,d
         CHARO   c#/s/,i    ;cout << "score = "
         CHARO   c#/c/,i
         CHARO   c#/o/,i
         CHARO   c#/r/,i
         CHARO   c#/e/,i
         CHARO   c#/ /,i
         CHARO   c#/=/,i
         CHARO   c#/ /,i
         DECO    score,d    ;   << score
         STOP
         .END

