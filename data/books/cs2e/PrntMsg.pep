;------- Print subroutine -------
;Prints a string of ASCII bytes until it encounters a null
;byte (eight zero bits).  Assumes one parameter, which
;contains the address of the message.
;
MsgAddr: .EQUATE d#2        ;Address of message to print
;
PrntMsg: LOADB   MsgAddr,s  ;B := address of message
         LOADX   d#0,i      ;X := 0
         LOADA   d#0,i      ;A := 0
PrntMore:LDBYTA  ,x         ;Test next char from Mem [B + X]
         BREQ    StopPrnt   ;If null then exit
         CHARO   ,x         ;else print
         ADDX    d#1,i      ;X := X + 1 for next character
         BR      PrntMore
StopPrnt:RTS   
         .END   
