;Program 6.16
         BR      Main
guess:   .BLOCK  d#2
;
;------- main()
Main:    LOADA   msgPrmt,i  ;cout << "Pick a number 0..3: "
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg
         ADDSP   d#2,i
         DECI    guess,d    ;cin >> Guess
         LOADB   jTable,i   ;B := address of jTable
         LOADX   guess,d    ;switch (Guess)
         ASLX               ;Addresses occupy two bytes
         BR      ,x
jTable:  .ADDRSS Case0
         .ADDRSS Case1
         .ADDRSS Case2
         .ADDRSS Case3
Case0:   LOADA   msg0,i     ;cout << "Not close"
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg
         ADDSP   d#2,i
         BR      EndCase    ;break
Case1:   LOADA   msg1,i     ;cout << "Close"
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg
         ADDSP   d#2,i
         BR      EndCase    ;break
Case2:   LOADA   msg2,i     ;cout << "Right on"
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg
         ADDSP   d#2,i
         BR      EndCase    ;break
Case3:   LOADA   msg3,i     ;cout << "Too high"
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg
         ADDSP   d#2,i
EndCase: STOP
msgPrmt: .ASCII  /Pick a number 0..3: /
         .BYTE   h#00
msg0:    .ASCII  /Not close/
         .BYTE   h#00
msg1:    .ASCII  /Close/
         .BYTE   h#00
msg2:    .ASCII  /Right on/
         .BYTE   h#00
msg3:    .ASCII  /Too high/
         .BYTE   h#00
;
;------- Print subroutine
;Prints a string of ASCII bytes until it encounters a null
;byte (eight zero bits).  Assumes one parameter, which
;contains the address of the message.
;
msgAddr: .EQUATE d#2        ;Address of message to print
;
PrntMsg: LOADB   msgAddr,s  ;B := address of message
         LOADX   d#0,i      ;X := 0
         LOADA   d#0,i      ;A := 0
PrntMore:LDBYTA  ,x         ;Test next char from Mem[B + X]
         BREQ    StopPrnt   ;If null then exit
         CHARO   ,x         ;else print
         ADDX    d#1,i      ;X := X + 1 for next character
         BR      PrntMore
StopPrnt:RTS
         .END

