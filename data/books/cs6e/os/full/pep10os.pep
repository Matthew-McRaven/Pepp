;******* Pep/9 Operating System, 2019/03/03
;
;
;******* Operating system RAM
         .SECTION "stack", "rwz"
osRAM:   .BLOCK  128         ;System stack area
wordTemp:.BLOCK  1           ;Temporary word storage #1h
byteTemp:.BLOCK  1           ;Least significant byte of wordTemp #1h
osSPTemp:.BLOCK  2           ;Store system stack pointer when calling user program #2h.
addrMask:.BLOCK  2           ;Addressing mode mask #2h.
opAddr:  .BLOCK  2           ;Trap instruction operand address #2h.

true:    .EQUATE 1
false:   .EQUATE 0
         .EXPORT true
         .EXPORT false
;******* Operating system ROM
         .SECTION "text", "rx"
;
;******* System Entry Point
retVal:  .EQUATE 0           ;Main return value #2d
disp:    MOVSPA              ;@bp@onload@once@cond#$ifOSDebug
         STWA    initSp,d    ;Preserve system stack pointer
         LDWA    osRAM,i     ;Load address of user stack
         MOVASP
         SUBSP   2,i         ;Allocate @param #retVal
         LDWA    0,i         ;Initialize user main return
         STWA    0,s         ;  value to zero
         LDWX    0,i         ;Initialize X to zero
         MOVAFLG             ;Initialize status bits to zero
         CALL    0x0000      ;Call main entry point
mainCln: LDWA    0,s         ;Load return value
         BRNE    mainErr     ;If retval is not zero, report error
         ADDSP   2,i         ;Deallocate @param #retVal
         LDWA    initSp,d    ;Restore system stack pointer
         MOVASP              ;OS Stack might be clobbered during by syscalls
;Write an arbitrary value to the power off port to shutdown the computer.
shutdown:LDWA    0xDEAD,i
         STBA    pwrOff,d
hang:    BR      hang
;
mainErr: LDWA    execErr,i   ;Load the address of the loader error address.
         STWA    -2,s        ;Push address of error message
         SUBSP   2,i         ;Allocate @param #msgAddr
         CALL    prntMsg
         ADDSP   2,i         ;Allocate @param #msgAddr
;Return value is already on stack, no need to push additional copy.
         CALL    numPrint
         ADDSP   2,i         ;Deallocate @param #retVal
         BR      shutdown
execErr: .ASCII "Main failed with return value \0"
         .BLOCK  64          ;Padding for possible future modification
;
;******* Trap handler
;NZVC bits, A and X registers contain user data after SCALL.
;From an OS perspective, these registers are unitialized values,
;and must be explicity set to prevent impacts on OS correctness.
oldIR:   .EQUATE 9           ;Stack address of IR on trap
oldPC:   .EQUATE 5           ;Stack address of PC on trap
oldA:    .EQUATE 1           ;Stack address of A on trap
;
trap:    LDWA    0,i
         MOVAFLG             ;Clear user supplied NZVC bits
         LDWX    0,i
         LDWA    oldPC,s     ;Must increment program counter
         ADDA    2,i
         STWA    oldPC,s
         LDWX    oldA,s
         BRLT    trapErr     ;System calls must be non-negative
         CPWX    ESCJT, i
         BRGE    trapErr     ;System calls must be within the table
         ASLX                ;Multiply index by 2 for word size
         CALL    SCJT, x     ;
         SRET
;
trapErr: LDWA    scErrMsg,i  ;Load the address of the loader error message.
         STWA    -2,s        ;Push address of error message
         SUBSP   2,i         ;Allocate @param #msgAddr
         CALL    prntMsg
         ADDSP   2,i         ;Deallocate @param #msgAddr
         LDWA    oldA,s
         STWA    -2,s
         SUBSP   2,i         ;Allocate @param #num
         CALL    numPrint
         ADDSP   2,i         ;Allocate @param #num
         BR      shutdown
scErrMsg:.ASCII "Could not find system call \0"
;
;******* Assert valid trap addressing mode
oldIR4:  .EQUATE 13          ;oldIR + 4 with two return addresses
assertAd:LDBA    1,i         ;A <- 1
         LDBX    oldIR4,s    ;X <- OldIR
         ANDX    0x0007,i    ;Keep only the addressing mode bits
         BREQ    testAd      ;000 = immediate addressing
loop:    ASLA                ;Shift the 1 bit left
         SUBX    1,i         ;Subtract from addressing mode count
         BRNE    loop        ;Try next addressing mode
testAd:  ANDA    addrMask,d  ;AND the 1 bit with legal modes
         BREQ    addrErr
         RET                 ;Legal addressing mode, return
addrErr: LDBA    '\n',i
         STBA    charOut,d
         LDWA    trapMsg,i   ;Push address of error message
         STWA    -2,s
         SUBSP   2,i         ;Allocate @param #msgAddr
         CALL    prntMsg     ;Call print subroutine
         ADDSP   2,i         ;Deallocate @param #msgAddr
         BR      shutdown    ;Halt: Fatal runtime error
trapMsg: .ASCII  "ERROR: Invalid trap addressing mode.\0"
;
;******* Set address of trap operand
oldX4:   .EQUATE 7           ;oldX + 4 with two return addresses
oldPC4:  .EQUATE 9           ;oldPC + 4 with two return address
oldSP4:  .EQUATE 11          ;oldSP + 4 with two return address
oldOpr4: .EQUATE 14          ;oldOS + 4 with two return address
setAddr: LDBX    oldIR4,s    ;X <- old instruction register
         ANDX    0x0007,i    ;Keep only the addressing mode bits
         ASLX                ;Two bytes per address
         BR      addrJT,x
addrJT:  .WORD addrI       ;Immediate addressing
         .WORD addrD       ;Direct addressing
         .WORD addrN       ;Indirect addressing
         .WORD addrS       ;Stack-relative addressing
         .WORD addrSF      ;Stack-relative deferred addressing
         .WORD addrX       ;Indexed addressing
         .WORD addrSX      ;Stack-indexed addressing
         .WORD addrSFX     ;Stack-deferred indexed addressing
;
;We compute the address of operands rather than their values
;except for immediate, where we compute its value, which unifies I and D.
addrI:   .BLOCK  0           ;OprndSpec
addrD:   LDWX    oldOpr4,s   ;Direct addressing
         STWX    opAddr,d    ;Oprnd = Mem[OprndSpec]
         RET
;
addrN:   LDWX    oldOpr4,s   ;Indirect addressing
         LDWX    0,x         ;Oprnd = Mem[Mem[OprndSpec]]
         STWX    opAddr,d
         RET
;
addrS:   LDWX    oldOpr4,s   ;Stack-relative addressing
         ADDX    oldSP4,s    ;Oprnd = Mem[SP + OprndSpec]
         STWX    opAddr,d
         RET
;
addrSF:  LDWX    oldOpr4,s   ;Stack-relative deferred addressing
         ADDX    oldSP4,s    ;Oprnd = Mem[Mem[SP + OprndSpec]]
         LDWX    0,x
         STWX    opAddr,d
         RET
;
addrX:   LDWX    oldOpr4,s   ;Indexed addressing
         ADDX    oldX4,s     ;Oprnd = Mem[OprndSpec + X]
         STWX    opAddr,d
         RET
;
addrSX:  LDWX    oldOpr4,s   ;Stack-indexed addressing
         ADDX    oldX4,s     ;Oprnd = Mem[SP + OprndSpec + X]
         ADDX    oldSP4,s
         STWX    opAddr,d
         RET
;
addrSFX: LDWX    oldOpr4,s   ;Stack-deferred indexed addressing
         ADDX    oldSP4,s    ;Oprnd = Mem[Mem[SP + OprndSpec] + X]
         LDWX    0,x
         ADDX    oldX4,s
         STWX    opAddr,d
         RET
;System Call Jump Table
DECI:    .EQUATE 0
         .EXPORT DECI
SCJT:    .WORD   _DECI
DECO:    .EQUATE 1
         .EXPORT DECO
         .WORD   _DECO
HEXO:    .EQUATE 2
         .EXPORT HEXO
         .WORD   _HEXO
STRO:    .EQUATE 3
         .EXPORT STRO
         .WORD   _STRO
SNOP:    .EQUATE 4
         .EXPORT SNOP
         .WORD   _SNOP
ESCJT:   .EQUATE 5
;
;
;******* SNOP
;The no-operation system call.
         .SCALL  SNOP
_SNOP:   LDWA    0x0001,i    ;Assert i
         STWA    addrMask,d
         CALL    assertAd
         RET                 ;Return to trap handler

;******* DECI
;The decimal input system call.
;Input format: Any number of leading spaces or line feeds are
;allowed, followed by '+', '-' or a digit as the first character,
;after which digits are input until the first nondigit is
;encountered. The status flags N,Z and V are set appropriately
;by this DECI routine. The C status flag is not affected.
         .SCALL  DECI
;
oldNZVC: .EQUATE 15          ;Stack address of NZVC on interrupt
;
total:   .EQUATE 11          ;#2d Cumulative total of DECI number
asciiCh: .EQUATE 10          ;#1c asciiCh, one byte
valAscii:.EQUATE 8           ;#2d value(asciiCh)
isOvfl:  .EQUATE 6           ;#2d Overflow boolean
isNeg:   .EQUATE 4           ;#2d Negative boolean
state:   .EQUATE 2           ;#2d State variable
temp:    .EQUATE 0           ;#2d
;
init:    .EQUATE 0           ;Enumerated values for state
sign:    .EQUATE 1
digit:   .EQUATE 2
;
_DECI:   LDWA    0x00FE,i    ;Assert d, n, s, sf, x, sx, sfx
         STWA    addrMask,d
         CALL    assertAd
         CALL    setAddr     ;Set address of trap operand
         SUBSP   13,i        ;@locals#total#asciiCh#valAscii#isOvfl#isNeg#state#temp
         LDWA    false,i     ;isOvfl <- false
         STWA    isOvfl,s
         LDWA    init,i      ;state <- init
         STWA    state,s
;
do:      LDBA    charIn,d    ;Get asciiCh
         STBA    asciiCh,s
         ANDA    0x000F,i    ;Set value(asciiCh)
         STWA    valAscii,s
         LDBA    asciiCh,s   ;A<low> = asciiCh throughout the loop
         LDWX    state,s     ;switch (state)
         ASLX                ;Two bytes per address
         BR      stateJT,x
;
stateJT: .WORD sInit
         .WORD sSign
         .WORD sDigit
;
sInit:   CPBA    '+',i       ;if (asciiCh == '+')
         BRNE    ifMinus
         LDWX    false,i     ;isNeg <- false
         STWX    isNeg,s
         LDWX    sign,i      ;state <- sign
         STWX    state,s
         BR      do
;
ifMinus: CPBA    '-',i       ;else if (asciiCh == '-')
         BRNE    ifDigit
         LDWX    true,i      ;isNeg <- true
         STWX    isNeg,s
         LDWX    sign,i      ;state <- sign
         STWX    state,s
         BR      do
;
ifDigit: CPBA    '0',i       ;else if (asciiCh is a digit)
         BRLT    ifWhite
         CPBA    '9',i
         BRGT    ifWhite
         LDWX    false,i     ;isNeg <- false
         STWX    isNeg,s
         LDWX    valAscii,s  ;total <- value(asciiCh)
         STWX    total,s
         LDWX    digit,i     ;state <- digit
         STWX    state,s
         BR      do
;
ifWhite: CPBA    ' ',i       ;else if (asciiCh is not a space
         BREQ    do
         CPBA    '\n',i      ;or line feed)
         BRNE    deciErr     ;exit with DECI error
         BR      do
;
sSign:   CPBA    '0',i       ;if asciiCh (is not a digit)
         BRLT    deciErr
         CPBA    '9',i
         BRGT    deciErr     ;exit with DECI error
         LDWX    valAscii,s  ;else total <- value(asciiCh)
         STWX    total,s
         LDWX    digit,i     ;state <- digit
         STWX    state,s
         BR      do
;
sDigit:  CPBA    '0',i       ;if (asciiCh is not a digit)
         BRLT    deciNorm
         CPBA    '9',i
         BRGT    deciNorm    ;exit normaly
         LDWX    true,i      ;else X <- true for later assignments
         LDWA    total,s     ;Multiply total by 10 as follows:
         ASLA                ;First, times 2
         BRV     ovfl1       ;If overflow then
         BR      L1
ovfl1:   STWX    isOvfl,s    ;isOvfl <- true
L1:      STWA    temp,s      ;Save 2 * total in temp
         ASLA                ;Now, 4 * total
         BRV     ovfl2       ;If overflow then
         BR      L2
ovfl2:   STWX    isOvfl,s    ;isOvfl <- true
L2:      ASLA                ;Now, 8 * total
         BRV     ovfl3       ;If overflow then
         BR      L3
ovfl3:   STWX    isOvfl,s    ;isOvfl <- true
L3:      ADDA    temp,s      ;Finally, 8 * total + 2 * total
         BRV     ovfl4       ;If overflow then
         BR      L4
ovfl4:   STWX    isOvfl,s    ;isOvfl <- true
L4:      ADDA    valAscii,s  ;A <- 10 * total + valAscii
         BRV     ovfl5       ;If overflow then
         BR      L5
ovfl5:   STWX    isOvfl,s    ;isOvfl <- true
L5:      STWA    total,s     ;Update total
         BR      do
;
deciNorm:LDWA    isNeg,s     ;If isNeg then
         BREQ    setNZ
         LDWA    total,s     ;If total != 0x8000 then
         CPWA    0x8000,i
         BREQ    L6
         NEGA                ;Negate total
         STWA    total,s
         BR      setNZ
L6:      LDWA    false,i     ;else -32768 is a special case
         STWA    isOvfl,s    ;isOvfl <- false
;
setNZ:   LDBX    oldNZVC,s   ;Set NZ according to total result:
         ANDX    0x0001,i    ;First initialize NZV to 000
         LDWA    total,s     ;If total is negative then
         BRGE    checkZ
         ORX     0x0008,i    ;set N to 1
checkZ:  CPWA    0,i         ;If total is not zero then
         BRNE    setV
         ORX     0x0004,i    ;set Z to 1
setV:    LDWA    isOvfl,s    ;If not isOvfl then
         BREQ    storeFl
         ORX     0x0002,i    ;set V to 1
storeFl: STBX    oldNZVC,s   ;Store the NZVC flags
;
exitDeci:LDWA    total,s     ;Put total in memory
         STWA    opAddr,n
         ADDSP   13,i        ;@locals#temp#state#isNeg#isOvfl#valAscii#asciiCh#total
         RET                 ;Return to trap handler
;
deciErr: LDBA    '\n',i
         STBA    charOut,d
         LDWA    deciMsg,i   ;Push address of message onto stack
         STWA    -2,s
         SUBSP   2,i         ;Allocate @param #msgAddr
         CALL    prntMsg     ;and print
         ADDSP   2,i         ;Deallocate @param #msgAddr
         BR      shutdown    ;Fatal error: program terminates
;
deciMsg: .ASCII  "ERROR: Invalid DECI input\0"
;
;******* DECO
;The decimal output system call.
;Output format: If the operand is negative, the algorithm prints
;a single '-' followed by the magnitude. Otherwise it prints the
;magnitude without a leading '+'. It suppresses leading zeros.
         .SCALL  DECO
;
_DECO:   LDWA    0x00FF,i    ;Assert i, d, n, s, sf, x, sx, sfx
         STWA    addrMask,d
         CALL    assertAd
         CALL    setAddr     ;Set address of trap operand
         LDWA    opAddr,n    ;A <- oprnd
         STWA    -2,s
         SUBSP   2,i         ;Allocate @param #toPrint
         CALL    numPrint
         ADDSP   2,i         ;Deallocate @param #toPrint
         RET                 ;Return to trap handler
;Print number
;Expects the number to be printed stored in the accumulator.
remain:  .EQUATE 0           ;#2d Remainder of value to output
outYet:  .EQUATE 2           ;#2d Has a character been output yet?
place:   .EQUATE 4           ;#2d Place value for division
toPrint: .EQUATE 8           ;#2d Number to be printed
numPrint:SUBSP   6,i         ;Allocate @locals #remain#outYet#place
         LDWA    toPrint,s   ;Load the number to print
         CPWA    0,i         ;If oprnd is negative then
         BRGE    printMag
         LDBX    '-',i       ;Print leading '-'
         STBX    charOut,d
         NEGA                ;Make magnitude positive
printMag:STWA    remain,s    ;remain <- abs(oprnd)
         LDWA    false,i     ;Initialize outYet <- false
         STWA    outYet,s
         LDWA    10000,i     ;place <- 10,000
         STWA    place,s
         CALL    divide      ;Write 10,000's place
         LDWA    1000,i      ;place <- 1,000
         STWA    place,s
         CALL    divide      ;Write 1000's place
         LDWA    100,i       ;place <- 100
         STWA    place,s
         CALL    divide      ;Write 100's place
         LDWA    10,i        ;place <- 10
         STWA    place,s
         CALL    divide      ;Write 10's place
         LDWA    remain,s    ;Always write 1's place
         ORA     0x0030,i    ;Convert decimal to ASCII
         STBA    charOut,d   ;  and output it
         ADDSP   6,i         ;Deallocate @locals #place#outYet#remain
         RET
;
;Subroutine to print the most significant decimal digit of the
;remainder. It assumes that place (place2 here) contains the
;decimal place value. It updates the remainder.
;
remain2: .EQUATE 2           ;Stack addresses while executing a
outYet2: .EQUATE 4           ;  subroutine are greater by two because
place2:  .EQUATE 6           ;  the retAddr is on the stack
;
divide:  LDWA    remain2,s   ;A <- remainder
         LDWX    0,i         ;X <- 0
divLoop: SUBA    place2,s    ;Division by repeated subtraction
         BRLT    writeNum    ;If remainder is negative then done
         ADDX    1,i         ;X <- X + 1
         STWA    remain2,s   ;Store the new remainder
         BR      divLoop
;
writeNum:CPWX    0,i         ;If X != 0 then
         BREQ    checkOut
         LDWA    true,i      ;outYet <- true
         STWA    outYet2,s
         BR      printDgt    ;and branch to print this digit
checkOut:LDWA    outYet2,s   ;else if a previous char was output
         BRNE    printDgt    ;then branch to print this zero
         RET                 ;else return to calling routine
;
printDgt:ORX     0x0030,i    ;Convert decimal to ASCII
         STBX    charOut,d   ;  and output it
         RET                 ;return to calling routine
;
;******* HEXO
;The hexadecimal ouput system call.
;Outputs one word as four hex characters from memory.
         .SCALL  HEXO
;
_HEXO:   LDWA    0x00FF,i    ;Assert i, d, n, s, sf, x, sx, sfx
         STWA    addrMask,d
         CALL    assertAd
         CALL    setAddr     ;Set address of trap operand
         LDWA    opAddr,n    ;A <- oprnd
         STWA    -2,s
         SUBSP   2,i         ;Allocate @param #num
         CALL    hexPrint
         ADDSP   2,i         ;Deallocate @param #num
         RET
num:             .EQUATE 2   ;#2h
hexPrint:LDWA    num,s
         STWA    wordTemp,d  ;Save oprnd in wordTemp
         LDBA    wordTemp,d  ;Put high-order byte in low-order A
         @ASRA4              ;Shift right four bits
         CALL    hexOut      ;Output first hex character
         LDBA    wordTemp,d  ;Put high-order byte in low-order A
         CALL    hexOut      ;Output second hex character
         LDBA    byteTemp,d  ;Put low-order byte in low order A
         @ASRA4              ;Shift right four bits
         CALL    hexOut      ;Output third hex character
         LDBA    byteTemp,d  ;Put low-order byte in low order A
         CALL    hexOut      ;Output fourth hex character
         RET
;
;Subroutine to output in hex the least significant nybble of the
;accumulator.
;
hexOut:  ANDA    0x000F,i    ;Isolate the digit value
         CPBA    9,i         ;If it is not in 0..9 then
         BRLE    prepNum
         SUBA    9,i         ;  convert to ASCII letter
         ORA     0x0040,i    ;  and prefix ASCII code for letter
         BR      writeHex
prepNum: ORA     0x0030,i    ;else prefix ASCII code for number
writeHex:STBA    charOut,d   ;Output nybble as hex
         RET
;
;******* STRO
;The string output system call.
;Outputs a null-terminated string from memory.
         .SCALL  STRO
;
_STRO:   LDWA    0x003E,i    ;Assert d, n, s, sf, x
         STWA    addrMask,d
         CALL    assertAd
         CALL    setAddr     ;Set address of trap operand
         LDWA    opAddr,d    ;Push address of string to print
         STWA    -2,s
         SUBSP   2,i         ;Allocate @param #msgAddr
         CALL    prntMsg     ;and print
         ADDSP   2,i         ;Deallocate @param #msgAddr
         RET                 ;Return to trap handler
;
;******* Print subroutine
;Prints a string of ASCII bytes until it encounters a null
;byte (eight zero bits). Assumes one parameter, which
;contains the address of the message.
;
msgAddr: .EQUATE 2           ;#2h Address of message to print
;
prntMsg: LDWX    0,i         ;X <- 0
         LDWA    0,i         ;A <- 0
prntMore:LDBA    msgAddr,sfx ;Test next char
         BREQ    exitPrnt    ;If null then exit
         STBA    charOut,d   ;else print
         ADDX    1,i         ;X <- X + 1 for next character
         BR      prntMore
;
exitPrnt:RET
         .BLOCK  64          ;Padding for possible future modification
;
;******* Vectors for system memory map
;
trpHnd:  .WORD  trap        ;Address of first instruction in trap handler.
initPC:  .WORD  disp        ;Address of first instruction to execute on boot.
         .SECTION "memvec", "rw"
         .ORG   0xFFFB
initSp:  .WORD  wordTemp    ;Initial stack pointer. Must be updated before
                             ; calling main or OS state will be clobbered.

         .EXPORT charIn      ;Allow charIn to be referenced in user programs.
         .INPUT  charIn      ;Mark charIn as a memory-mapped input device
charIn:  .BLOCK  1           ;Memory-mapped input device #1h.

         .EXPORT charOut     ;Allow charOut to be referenced in user programs.
         .OUTPUT charOut     ;Mark charOut as a memory-mapped output device
charOut: .BLOCK  1           ;Memory-mapped output device #1h.

         .OUTPUT pwrOff      ;Mark pwfOff as a memory-mapped output device
         .EXPORT pwrOff      ;Allow pwrOff to be referenced in user programs.
pwrOff:  .BLOCK  1           ;Memory-mapped shutdown device #1h.
