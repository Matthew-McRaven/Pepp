;Program 6.12
         LOADB   msg,i      ;B := address of msg
         LOADX   d#0,i      ;X := 0
         LOADA   d#0,i      ;A := 0
PrntMore:LDBYTA  ,x         ;test Mem[msg + X]
         BREQ    Halt       ;if null then exit
         CHARO   ,x         ;output next char
         ADDX    d#1,i      ;X := X + 1
         BR      PrntMore   ;repeat the loop
Halt:    STOP
msg:     .ASCII  /A long message./
         .BYTE   h#00
         .END

