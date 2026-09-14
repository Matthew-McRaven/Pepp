;Program 6.4
         BR      Main
cop:     .BLOCK  d#2
driver:  .BLOCK  d#2
;
Main:    LOADA   d#0,i      ;cop = 0
         STOREA  cop,d
         LOADA   d#40,i     ;driver = 40
         STOREA  driver,d
Do:      LOADA   cop,d      ;cop += 25
         ADDA    d#25,i
         STOREA  cop,d
         LOADA   driver,d   ;driver += 20
         ADDA    d#20,i
         STOREA  driver,d
While:   LOADA   cop,d      ;while (cop < driver)
         COMPA   driver,d
         BRLT    Do
         DECO    cop,d      ;cout << cop
         STOP
         .END

