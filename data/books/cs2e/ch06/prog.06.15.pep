;Program 6.15
         BR      Main
nums:    .BLOCK  d#6

i:       .BLOCK  d#2
;
;------- void GetNums (int list[])
list:    .EQUATE d#4        ;formal parameter
j:       .EQUATE d#0        ;local variable
GetNums: ADDSP   d#-2,i     ;allocate local
         LOADB   list,s     ;B := list (an address)
         LOADX   d#0,i      ;initialize j = 0
         STOREX  j,s
For1:    COMPX   d#3,i      ;test for j < 3
         BRGE    EndFor1
         ASLX               ;integers occupy two bytes
         DECI    ,x         ;cin >> list[j]
         LOADX   j,s        ;increment j++
         ADDX    d#1,i
         STOREX  j,s
         BR      For1
EndFor1: ADDSP   d#2,i      ;deallocate local
         RTS
;
;------- main()
Main:    LOADA   msg1,i     ;cout << "Enter three numbers:

         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg
         ADDSP   d#2,i
;
         LOADA   nums,i     ;GetNums(nums)
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     GetNums
         ADDSP   d#2,i
;
         LOADA   msg2,i     ;cout << "Reverse order: "
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg
         ADDSP   d#2,i
;
         LOADB   nums,i     ;B := nums (an address)
         LOADX   d#2,i      ;initialize i = 2
         STOREX  i,d
For2:    BRLT    EndFor2    ;test for i >= 0
         ASLX               ;integers occupy two bytes
         DECO    ,x         ;cout << nums[i] << ' '
         CHARO   c#/ /,i
         LOADX   i,d        ;decrement i--
         SUBX    d#1,i
         STOREX  i,d
         BR      For2
EndFor2: STOP
msg1:    .ASCII  /Enter three numbers: /
         .BYTE   h#00
msg2:    .ASCII  /Reverse order: /
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

