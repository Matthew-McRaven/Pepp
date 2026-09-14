;Program 6.11
         LOADA   d#0,i      ;A := 0
PrntMore:LDBYTA  msg,d      ;test char
         BREQ    Halt       ;if null then exit
         CHARO   msg,d      ;output next char
         LOADA   h#0004,d   ;get LDBYTA OprndSpec
         ADDA    d#1,i      ;increment it
         STOREA  h#0004,d   ;modify LDBYTA OprndSpec
         STOREA  h#000A,d   ;modify CHARO OprndSpec
         BR      PrntMore   ;repeat the loop
Halt:    STOP
msg:     .ASCII  /A long message./
         .BYTE   h#00
         .END

