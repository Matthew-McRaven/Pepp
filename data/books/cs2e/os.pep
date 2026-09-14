;
;******* Pep/7 Operating System, 5/2/00
;
lineFeed:.EQUATE h#000A     ;ASCII line feed character
space:   .EQUATE h#0020     ;ASCII space character
TRUE:    .EQUATE d#1
FALSE:   .EQUATE d#0
;
;******* Operating system RAM
osRAM:   .BLOCK  d#100      ;System stack area
wordBuff:.BLOCK  d#1        ;Input/output buffer
byteBuff:.BLOCK  d#1        ;Least significant byte of wordBuff
wordTemp:.BLOCK  d#1        ;Temporary word storage
byteTemp:.BLOCK  d#1        ;Least significant byte of tempWord
isUnary: .BLOCK  d#2        ;Boolean flag
;
;******* Operating system ROM
         .BURN   h#7FFF     ;Version OS/7.0
;
;======= System loader
;Loader format rules
;This loader assumes that all data in the object file to be
;loaded is in the following strict format:  Each hex number
;representing a byte must contain exactly two characters, each
;character must be in 0..9, A..F, or a..f and must be followed
;by exactly one space.  No leading spaces are permitted at the
;beginning of a line.  No trailing spaces are permitted at the
;end of a line.  The last two characters in the file must be
;lowercase zz, which is used as a terminating sentinel.  To
;improve execution speed, this loader does not trap input
;errors, and will not function properly unless the file input
;format is consistent with these restrictions.
;
Loader:  LOADB   d#0,i      ;Clear base for memory store
         LOADX   d#0,i      ;Initialize index register
         STOREX  wordBuff,d ;Clear input buffer word
;
GetChar: CHARI   byteBuff,d ;Get first hex digit
         LOADA   wordBuff,d ;Put ASCII in low byte of A
         COMPA   c#/z/,i    ;If end of file sentinel, 'z',
         BREQ    StopLoad   ;then exit loader routine
         COMPA   c#/9/,i    ;If ASCII <= '9', then assume decimal
         BRLE    Shift      ;and right nybble is correct number
         ADDA    d#9,i      ;else convert nybble to correct number
Shift:   ASLA               ;Shift left by four bits, sending the
         ASLA               ;number to the most significant
         ASLA               ;nybble position in the byte
         ASLA  
         STBYTA  byteTemp,d ;Save most significant nybble
         CHARI   byteBuff,d ;Get second hex digit
         LOADA   wordBuff,d ;Put second ASCII in low byte of A
         COMPA   c#/9/,i    ;If ASCII <= '9', then assume decimal
         BRLE    Combine    ;and right nybble is correct number
         ADDA    d#9,i      ;else convert nybble to correct number
Combine: ANDA    h#000F,i   ;Mask out the unwanted left nybble
         ORA     wordTemp,d ;Combine both hex digits in binary
         STBYTA  ,x         ;Store in Mem[B + X]
         ADDX    d#1,i      ;Increment index register
         CHARI   byteBuff,d ;Skip blank or <LF>
         BR      GetChar
;
StopLoad:STOP  
;
;======= Interrupt service routine
oldPC:   .EQUATE d#7        ;Stack address of PC on interrupt
oldIR:   .EQUATE d#11       ;Stack address of IR on interrupt
;
Intrupt: LDBYTX  oldIR,s    ;X := intrpt IR
         ASRX               ;Discard addressing mode
         ASRX               ;Discard addressing mode
         ASRX               ;Discard register specifier
         ANDX    h#0003,i   ;Keep rightmost two bits of op-code
         SUBX    d#1,i      ;Adjust X from 1..3 to 0..2
         ASLX               ;Times 2 since an address occupies
         LOADB   jTable,i   ;two bytes
         JSR     ,x         ;Call Opcode routine
;
         LOADA   isUnary,d  ;If nonunary then
         BRNE    Return     ;increment PC by additional two 
         LOADA   oldPC,s    ;A := intrpt PC
         ADDA    d#2,i      ;Increment PC by two
         STOREA  oldPC,s    ;Restore PC for return
Return:  RTI   
;
jTable:  .ADDRSS Opc11101
         .ADDRSS Opc11110
         .ADDRSS Opc11111
;
;------- Opcode  11101
;The DECI instruction.
;Input format:  Any number of leading spaces or line feeds are
;allowed, followed by '+', '-' or a digit as the first character,
;after which digits are input until the first nondigit is
;encountered.  The status flags N, Z and V are set appropriately
;by this DECI routine.  The C status flag is not affected.
;
oldNZVC: .EQUATE d#14       ;Stack address of NZVC on interrupt
;
total:   .EQUATE d#10       ;Cumulative total of DECI number
valAscii:.EQUATE d#8        ;Value(asciiCh)
isOvfl:  .EQUATE d#6        ;Overflow boolean
isNeg:   .EQUATE d#4        ;Negative boolean
state:   .EQUATE d#2        ;State variable
temp:    .EQUATE d#0        ;Temporary
;
init:    .EQUATE d#0        ;Enumerated values for state
sign:    .EQUATE d#1
digit:   .EQUATE d#2
;
Opc11101:ADDSP   d#-12,i    ;Allocate storage for locals
         LOADA   FALSE,i    ;isOvfl := FALSE
         STOREA  isOvfl,s
         LOADA   init,i     ;state := init
         STOREA  state,s
         LOADA   d#0,i      ;wordBuff := 0 for input
         STOREA  wordBuff,d
         LOADB   sTable,i   ;For switch jump table on state
;
Do:      CHARI   byteBuff,d ;Get asciiCh
         LOADA   wordBuff,d ;Set Value(asciiCh)
         ANDA    h#000F,i
         STOREA  valAscii,s
         LOADA   wordBuff,d ;A contains asciiCh throughout loop
         LOADX   state,s    ;switch (state)
         ASLX               ;An address occupies two bytes
         BR      ,x
;
sTable:  .ADDRSS SInit
         .ADDRSS SSign
         .ADDRSS SDigit
;
SInit:   COMPA   c#/+/,i    ;if (asciiCh == '+')
         BRNE    IfMinus
         LOADX   FALSE,i    ;isNeg := FALSE
         STOREX  isNeg,s
         LOADX   sign,i     ;state := sign
         STOREX  state,s
         BR      Do
;
IfMinus: COMPA   c#/-/,i    ;else if (asciiCh == '-')
         BRNE    Ifdigit
         LOADX   TRUE,i     ;isNeg := TRUE
         STOREX  isNeg,s
         LOADX   sign,i     ;state := sign
         STOREX  state,s
         BR      Do
;
Ifdigit: COMPA   c#/0/,i    ;else if (asciiCh is a digit)
         BRLT    IfWhite
         COMPA   c#/9/,i
         BRGT    IfWhite
         LOADX   FALSE,i    ;isNeg := FALSE
         STOREX  isNeg,s
         LOADX   valAscii,s ;total := Value(asciiCh)
         STOREX  total,s
         LOADX   digit,i    ;state := digit
         STOREX  state,s
         BR      Do
;
IfWhite: COMPA   space,i    ;else if (asciiCh is not a space
         BREQ    Do
         COMPA   lineFeed,i ;or line feed)
         BRNE    DeciErr    ;exit with DECI error
         BR      Do
;
SSign:   COMPA   c#/0/,i    ;if asciiCh (is not a digit)
         BRLT    DeciErr
         COMPA   c#/9/,i
         BRGT    DeciErr    ;exit with DECI error
         LOADX   valAscii,s ;else total := Value(asciiCh)
         STOREX  total,s
         LOADX   digit,i    ;state := digit
         STOREX  state,s
         BR      Do
;
SDigit:  COMPA   c#/0/,i    ;if (asciiCh is not a digit)
         BRLT    DeciNorm
         COMPA   c#/9/,i
         BRGT    DeciNorm   ;exit normally
         LOADX   TRUE,i     ;else X := TRUE for later assignments
         LOADA   total,s    ;Multiply total by 10 as follows:
         ASLA               ;First, times 2
         BRV     Ovfl1      ;If overflow then
         BR      L1
Ovfl1:   STOREX  isOvfl,s   ;isOvfl := TRUE
L1:      STOREA  temp,s     ;Save 2 * total in temp
         ASLA               ;Now, 4 * total
         BRV     Ovfl2      ;If overflow then
         BR      L2
Ovfl2:   STOREX  isOvfl,s   ;isOvfl := TRUE
L2:      ASLA               ;Now, 8 * total
         BRV     Ovfl3      ;If overflow then
         BR      L3
Ovfl3:   STOREX  isOvfl,s   ;isOvfl := TRUE
L3:      ADDA    temp,s     ;Finally, 8 * total + 2 * total
         BRV     Ovfl4      ;If overflow then
         BR      L4
Ovfl4:   STOREX  isOvfl,s   ;isOvfl := TRUE
L4:      ADDA    valAscii,s ;A := 10 * total + valAscii
         BRV     Ovfl5      ;If overflow then
         BR      L5
Ovfl5:   STOREX  isOvfl,s   ;isOvfl := TRUE
L5:      STOREA  total,s    ;Update total
         BR      Do
;
DeciNorm:LOADA   isNeg,s    ;If isNeg then
         BREQ    SetNZ
         LOADA   total,s    ;If total != h#8000 then
         COMPA   h#8000,i
         BREQ    L6
         NOTA               ;Negate total by taking two's comp
         ADDA    d#1,i
         STOREA  total,s
         BR      SetNZ
L6:      LOADA   FALSE,i    ;else -32768 is a special case
         STOREA  isOvfl,s   ;isOvfl := FALSE
;
SetNZ:   LDBYTX  oldNZVC,s  ;Set NZ according to total result:
         ANDX    h#0001,i   ;First, initialize NZV to 000 
         LOADA   total,s    ;If total is negative then
         BRGE    CheckZ
         ORX     h#0008,i   ;set N to 1
CheckZ:  COMPA   d#0,i      ;If total is not zero then
         BRNE    SetV
         ORX     h#0004,i   ;set Z to 1
SetV:    LOADA   isOvfl,s   ;If not isOvfl then
         BREQ    StoreFl
         ORX     h#0002,i   ;set V to 1
StoreFl: STBYTX  oldNZVC,s  ;Store the NZVC flags
;
ExitDeci:ADDSP   d#10,i     ;Deallocate all locals except total
         JSR     Put        ;Put total in memory
         ADDSP   d#2,i      ;Deallocate total
         RTS   
;
DeciErr: LOADA   msg1,i     ;Push address of message onto stack
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg    ;and print
         STOP               ;Fatal error: program terminates
;
msg1:    .ASCII  /ERROR:  Invalid DECI input/
         .BYTE   h#00
;
;------- Opcode  11110
;The DECO instruction.
;Output format:  If the operand is negative, the algorithm prints
;a single '-' followed by the magnitude.  Otherwise it prints the
;magnitude without a leading '+'.  It suppresses leading zeros.
;
number:  .EQUATE d#0        ;Value returned by function Get
;
remain:  .EQUATE d#0        ;Remainder of number
chOut:   .EQUATE d#2        ;Has a character been output yet?
place:   .EQUATE d#4        ;Place value of number for division
;
Opc11110:ADDSP   d#-2,i     ;Allocate storage for number
         JSR     Get        ;Get number from memory
         LOADA   number,s   ;A := number
         ADDSP   d#-4,i     ;Allocate storage for locals
         COMPA   d#0,i      ;If oprnd is negative then
         BRGE    PrntMag
         CHARO   c#/-/,i    ;Print leading '-'
         NOTA               ;and negate oprnd
         ADDA    d#1,i
PrntMag: STOREA  remain,s   ;remain := abs(oprnd)
         LOADA   FALSE,i    ;Initialize chOut := FALSE
         STOREA  chOut,s
         LOADA   d#10000,i  ;place := 10,000
         STOREA  place,s
         JSR     Divide     ;Write 10,000's place
         LOADA   d#1000,i   ;place := 1000
         STOREA  place,s
         JSR     Divide     ;Write 1000's place
         LOADA   d#100,i    ;place := 100
         STOREA  place,s
         JSR     Divide     ;Write 100's place
         LOADA   d#10,i     ;place := 10
         STOREA  place,s
         JSR     Divide     ;Write 10's place
         LOADA   remain,s   ;Always write 1's place
         ORA     h#0030,i   ;Convert decimal to ASCII
         STBYTA  byteBuff,d
         CHARO   byteBuff,d
         ADDSP   d#6,i      ;Deallocate storage for locals
         RTS   
;
;Subroutine to print the most significant decimal digit of the
;remainder.  It assumes that place (place2 here) contains the
;decimal place value.  It updates the remainder.
;
remain2: .EQUATE d#2        ;Stack addresses while executing a
chOut2:  .EQUATE d#4        ;subroutine are greater by two because
place2:  .EQUATE d#6        ;the ret addr is on the stack
;
Divide:  LOADA   remain2,s  ;A := remainder
         LOADX   d#0,i      ;X := 0
DivLoop: SUBA    place2,s   ;Division by repeated subtraction
         BRLT    WriteNum   ;If remainder is negative, then done
         ADDX    d#1,i      ;X := X + 1
         STOREA  remain2,s  ;Store new remainder
         BR      DivLoop
;
WriteNum:COMPX   d#0,i      ;If X != 0 then
         BREQ    CheckOut
         LOADA   TRUE,i     ;chOut := TRUE
         STOREA  chOut2,s
         BR      PrntDgt    ;and branch to print this digit
CheckOut:LOADA   chOut2,s   ;else if a previous char was output
         BRNE    PrntDgt    ;then branch to print this zero
         RTS                ;else return to calling routine
;
PrntDgt: ORX     h#0030,i   ;Convert decimal to ASCII
         STOREX  wordBuff,d ;temporary for output
         CHARO   byteBuff,d
         RTS                ;Return to calling routine
;
;------- Opcode  11111
;The HEXO instruction.
;Output format:  Outputs one word as four hex characters.  number
;is defined in the DECO interrupt handler.
;
Opc11111:ADDSP   d#-2,i     ;Allocate storage for number
         JSR     Get        ;Get number from memory
         LOADA   number,s   ;A := number
         ADDSP   d#2,i      ;Deallocate storage for number
         STOREA  wordTemp,d ;Save oprnd
         LDBYTA  wordTemp,d ;Put high order byte in low order A
         ASRA               ;Shift right four bits
         ASRA  
         ASRA  
         ASRA  
         JSR     OutputAc   ;Output first hex character
         LDBYTA  wordTemp,d ;Put high order byte in low order A
         JSR     OutputAc   ;Output second hex character
         LDBYTA  byteTemp,d ;Put low order byte in low order A
         ASRA               ;Shift right four bits
         ASRA  
         ASRA  
         ASRA  
         JSR     OutputAc   ;Output third hex character
         LDBYTA  byteTemp,d ;Put low order byte in low order A
         JSR     OutputAc   ;Output fourth hex character
         RTS   
;
;Subroutine to output in hex the least significant nybble of the
;accumulator.
;
OutputAc:ANDA    h#000F,i   ;Isolate digit value
         COMPA   d#9,i      ;If it is not in 0..9 then
         BRLE    PrepNum
         SUBA    d#9,i      ;convert number to ASCII letter
         ORA     h#0040,i   ;and prefix ASCII code for letter
         BR      WriteHex
PrepNum: ORA     h#0030,i   ;else prefix ASCII code for number
WriteHex:STBYTA  byteBuff,d ;For output
         CHARO   byteBuff,d
         RTS   
;
;======= Put/Addressing mode decoder
;Put is a subroutine that DECI calls at the end of its input
;processing.  It decodes the addressing mode of the interrupted
;instruction, calculates the address of the operand of the
;interrupted instruction and puts the value of oprnd at that
;location in memory.
;
oprnd:   .EQUATE d#2        ;Value of formal parameter stored
;
oldNZVC6:.EQUATE d#6        ;Stack addresses of the register
oldA6:   .EQUATE d#7        ;contents while executing this
oldX6:   .EQUATE d#9        ;routine are greater by six because
oldB6:   .EQUATE d#11       ;two return addresses and a formal
oldPC6:  .EQUATE d#13       ;parameter are on the stack
oldSP6:  .EQUATE d#15
oldIR6:  .EQUATE d#17
;
Put:     LDBYTX  oldIR6,s   ;A := intrpt IR
         ANDX    h#0003,i   ;Retain only address mode bits.
         ASLX               ;An address occupies two bytes
         LOADB   jTable2,i
         BR      ,x
jTable2: .ADDRSS Immed2
         .ADDRSS Direct2
         .ADDRSS StkRel2
         .ADDRSS Indexed2
;
Immed2:  LOADA   msg2,i     ;Push address of message onto stack
         STOREA  d#-2,s
         ADDSP   d#-2,i
         JSR     PrntMsg    ;and print
         STOP               ;Fatal error: program terminates
;
Direct2: LOADA   oprnd,s    ;A := oprnd
         LOADB   oldPC6,s   ;B := intrpt PC
         LOADX   d#0,i      ;X := 0
         LOADB   ,x         ;B := intrpt oprndSpec
         STOREA  ,x         ;Store in Mem[oprndSpec]
         LOADA   FALSE,i    ;isUnary := FALSE
         STOREA  isUnary,d
         RTS   
;
StkRel2: LOADA   oprnd,s    ;A := oprnd
         LOADB   oldPC6,s   ;B := intrpt PC
         LOADX   d#0,i      ;X := 0
         LOADB   ,x         ;B := intrpt oprndSpec
         LOADX   oldSP6,s   ;X := intrpt SP
         STOREA  ,x         ;Store in Mem[oprndSpec + SP]
         LOADA   FALSE,i    ;isUnary := FALSE
         STOREA  isUnary,d
         RTS   
;
Indexed2:LOADA   oprnd,s    ;A := oprnd
         LOADB   oldB6,s    ;B := intrpt B
         LOADX   oldX6,s    ;X := intrpt X
         STOREA  ,x         ;Store in Mem[B + X]
         LOADA   TRUE,i     ;isUnary := TRUE
         STOREA  isUnary,d
         RTS   
;
msg2:    .ASCII  /ERROR:  Illegal use of immediate addressing/
         .BYTE   h#00
;
;======= Get/Addressing mode decoder
;Get is a function called at the beginning of DECO and HEXO.  It
;decodes the addressing mode of the interrupted instruction,
;calculates the operand of the interrupted instruction and returns
;its value in oprnd.  The stack addresses are identical to those
;in the Put addressing mode decoder.
;
Get:     LDBYTX  oldIR6,s   ;X := intrpt IR
         ANDX    h#0003,i   ;Retain only address mode bits
         ASLX               ;An address is two bytes
         LOADB   jTable3,i
         BR      ,x
jTable3: .ADDRSS Immed3
         .ADDRSS Direct3
         .ADDRSS StkRel3
         .ADDRSS Indexed3
;
Immed3:  LOADB   oldPC6,s   ;B := intrpt PC
         LOADX   d#0,i      ;X := 0
         LOADA   ,x         ;A := intrpt oprndSpec
         STOREA  oprnd,s
         LOADA   FALSE,i    ;isUnary := FALSE
         STOREA  isUnary,d
         RTS   
;
Direct3: LOADB   oldPC6,s   ;B := intrpt PC
         LOADX   d#0,i      ;X := 0
         LOADB   ,x         ;B := intrpt oprndSpec
         LOADA   ,x         ;A := Mem[oprndSpec]
         STOREA  oprnd,s
         LOADA   FALSE,i    ;isUnary := FALSE
         STOREA  isUnary,d
         RTS   
;
StkRel3: LOADB   oldPC6,s   ;B := intrpt PC
         LOADX   d#0,i      ;X := 0
         LOADB   ,x         ;B := intrpt oprndSpec
         LOADX   oldSP6,s   ;X := intrpt SP
         LOADA   ,x         ;A := Mem[oprndSpec + SP]
         STOREA  oprnd,s
         LOADA   FALSE,i    ;isUnary := FALSE
         STOREA  isUnary,d
         RTS   
;
Indexed3:LOADB   oldB6,s    ;B := intrpt B
         LOADX   oldX6,s    ;X := intrpt X
         LOADA   ,x         ;A := Mem[B + X]
         STOREA  oprnd,s
         LOADA   TRUE,i     ;isUnary := TRUE
         STOREA  isUnary,d
         RTS   
;
;======= Print subroutine
;Prints a line feed character followed by the string of ASCII
;bytes until it encounters a null byte (eight zero bits).  Assumes
;one parameter, which contains the address of the message.
;
msgAddr: .EQUATE d#2        ;Address of message to print
;
PrntMsg: CHARO   lineFeed,i
         LOADB   msgAddr,s  ;B := address of message
         LOADX   d#0,i      ;X := 0
         LOADA   d#0,i      ;A := 0
PrntMore:LDBYTA  ,x         ;Test next char from Mem[B + X]
         BREQ    StopPrnt   ;If null then exit
         CHARO   ,x         ;else print
         ADDX    d#1,i      ;X := X + 1 for next character
         BR      PrntMore
;
StopPrnt:RTS   
;
;======= Vectors reserved for machine use
         .ADDRSS osRAM      ;User stack pointer
         .ADDRSS wordBuff   ;System stack pointer
         .ADDRSS Loader     ;Loader program counter
         .ADDRSS Intrupt    ;Interrupt program counter
;
         .END
