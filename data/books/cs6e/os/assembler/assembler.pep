         .SECTION "stack", "rwz"
pStack:  .BLOCK  128         ;Limit return stack to 128 bytes
rStack:  .BLOCK  2           ;Padding before IDE controller
;Memory-mapped IDE controller registers
; It uses 256B sectors instead of the usual 512B so that offsets in a sector can be 8 bits.
; It always use DMA to avoid the CPU overhead of programmed IO (PIO).
;We omit several registers from the IDE spec, with the assumption that
; the IDE controller will set them appropriately on our behalf. We omit:
; - the data register, since we will use DMA to transfer data.
; - LBA bits 16-27, limiting us to 64k sectors of data.
; - the sector count register, since we provide an additional "length" register measured in bytes.
; - the sector number register, since we will use LBA addressing.
; - the cylinder low/high registers, since we will use LBA addressing.
; - the drive/head register, since we will use LBA addressing
; - the status and error registers; we (foolishly) assume all operations complete
;   instantly and without errors.
;We add 3 additional registers:
; - a 2B DMA address register, specifying a base address in main memory.
; - a 2B transfer length register, representing the length of the DMA in bytes.
; - a 1B sector offset register, allowing a transfer to start an an arbitrary byte in the sector.
;The sector offset is primarily intended to allow sub-256B transfers within one sector.
; It is assumed that the underlying IDE device will require 256B transfers,
; so the controller will be responsible for a read-modify-write if needed.
;The memory-mapped registers will total to 8 bytes.
;
;0xC9 for READ, 0xCB for WRITE, 0x50 to erase (set to 0). All other values ignored.
ideCMD:  .BLOCK  1
         .EXPORT ideCMD
;Offset into the sector from which the transfer will start.
offLBA:  .BLOCK  1
         .EXPORT offLBA
;Combine lowLBA and hiLBA, and reverse order of registers WRT the spec so they are big endian.
LBA:     .BLOCK  2
         .EXPORT LBA
addrDMA: .BLOCK  2
         .EXPORT addrDMA
;Number of bytes to transfer via DMA. May be <256.
lenDMA:  .BLOCK  2
         .EXPORT lenDMA
         .BLOCK 2            ;Padding to protect IDE controller
_BUF:    .BLOCK  32          ;Static buffer into which WORD reads
;
         .SECTION "text", "rx"
;
;******* Reusable assembly routines
;All of these routines assume they can clobber A & X; callers are responsible for saving.
;String comparison
sEq:      .EQUATE 12           ;#1d Return val. 0 if equal, non-0 otherwise.
sLen1:    .EQUATE 10           ;#2d String 1 length
sPtr1:    .EQUATE 8            ;#2h String 1 pointer
sLen2:    .EQUATE 6            ;#2d String 2 length
sPtr2:    .EQUATE 4            ;#2h String 2 pointer
;         .EQUATE 2            ;#2d Return address
sMaxIdx:  .EQUATE 0            ;#2d Maximum index to compare
strCmp:   SUBSP   2,i          ;@locals#sMaxIdx
          LDWX    0,i          ;Initialize counter to 0
          LDWA    sLen1,s      ;sMaxIdx <- min(sLen1, sLen2)
          CPWA    sLen2,s
          BRGE    strcL1
          LDWA    sLen2,s
          STWA    sMaxIdx,s
          BR      strcLoop
strcL1:   STWA    sMaxIdx,s
;
strcLoop: LDBA    sPtr1,sfx    ;A <- sPtr1[X]
          CPBA    sPtr2,sfx    ;Z <- sPtr1[X] == sPtr2[X]
          BRNE    strcNE
          ADDX    1,i          ;X <- X + 1
          CPWX    sMaxIdx,s    ;If X == sMaxIdx then
          BREQ    strcEQ       ;  return suceess
          BR      strcLoop
strcNE:   LDBA    -1,i         ;sEq <- -1
          BR      strcRet
;
strcEQ:   LDBA    0,i          ;sEq <- 0
strcRet:  STBA    sEq,s
          ADDSP   2,i          ;@locals#sMaxIdx
          RET
;Input format: Any number of leading spaces or line feeds are
;allowed, followed by '+', '-' or a digit as the first character,
;after which digits are input until the first nondigit is
;encountered.
;

total:   .EQUATE 16          ;#2d Cumulative total of DECI number
success: .EQUATE 15          ;#1d Success boolean
bufIdx:  .EQUATE 12          ;#2d Index into _BUF
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
decRead: SUBSP   14,i        ;@locals#bufIdx#asciiCh#valAscii#isOvfl#isNeg#state#temp
         LDWA    0,i         ;isOvfl <- false
         STWA    isOvfl,s
         STWA    bufIdx,s    ;bufIdx <- 0
         LDWA    init,i      ;state <- init
         STWA    state,s
;
deciDo:  LDWX    bufIdx,s    ;Get bufIdx
         LDBA    _BUF,x      ;Get asciiCh
         STBA    asciiCh,s
         ADDX    1,i         ;bufIdx <- bufIdx + 1
         STWX    bufIdx,s
         ANDA    0x000F,i    ;Set value(asciiCh)
         STWA    valAscii,s
         LDBA    asciiCh,s   ;A<low> = asciiCh throughout the loop
         LDWX    state,s     ;switch (state)
         ASLX                ;Two bytes per address
         BR      deciJT,x
;
deciJT:  .WORD sInit
         .WORD sSign
         .WORD sDigit
;
sInit:   CPBA    '+',i       ;if (asciiCh == '+')
         BRNE    ifMinus
         LDWX    0,i         ;isNeg <- false
         STWX    isNeg,s
         LDWX    sign,i      ;state <- sign
         STWX    state,s
         BR      deciDo
;
ifMinus: CPBA    '-',i       ;else if (asciiCh == '-')
         BRNE    ifDigit
         LDWX    1,i         ;isNeg <- true
         STWX    isNeg,s
         LDWX    sign,i      ;state <- sign
         STWX    state,s
         BR      deciDo
;
ifDigit: CPBA    '0',i       ;else if (asciiCh is a digit)
         BRLT    ifWhite
         CPBA    '9',i
         BRGT    ifWhite
         LDWX    0,i         ;isNeg <- false
         STWX    isNeg,s
         LDWX    valAscii,s  ;total <- value(asciiCh)
         STWX    total,s
         LDWX    digit,i     ;state <- digit
         STWX    state,s
         BR      deciDo
;
ifWhite: CPBA    ' ',i       ;else if (asciiCh is not a space
         BREQ    deciDo
         CPBA    '\n',i      ;or line feed)
         BRNE    deciErr     ;exit with DECI error
         BR      deciDo
;
sSign:   CPBA    '0',i       ;if asciiCh (is not a digit)
         BRLT    deciErr
         CPBA    '9',i
         BRGT    deciErr     ;exit with DECI error
         LDWX    valAscii,s  ;else total <- value(asciiCh)
         STWX    total,s
         LDWX    digit,i     ;state <- digit
         STWX    state,s
         BR      deciDo
;
sDigit:  CPBA    '0',i       ;if (asciiCh is not a digit)
         BRLT    deciNorm
         CPBA    '9',i
         BRGT    deciNorm    ;exit normaly
         LDWX    1,i         ;else X <- true for later assignments
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
         BR      deciDo
;
deciNorm:LDWA    isNeg,s     ;If isNeg then
         BREQ    exitDeci
         LDWA    total,s     ;If total != 0x8000 then
         CPWA    0x8000,i
         BREQ    L6
         NEGA                ;Negate total
         STWA    total,s
         BR      exitDeci
L6:      LDWA    0,i         ;else -32768 is a special case
         STWA    isOvfl,s    ;isOvfl <- false
;
deciErr: LDBA    0,i
         STBA    success,s   ;success <- false
exitDeci:ADDSP   14,i        ;@locals#temp#state#isNeg#isOvfl#valAscii#asciiCh#bufIdx
         RET                 ;Return
;
;Output format: If the operand is negative, the algorithm prints
;a single '-' followed by the magnitude. Otherwise it prints the
;magnitude without a leading '+'. It suppresses leading zeros.
;
;Print number
;Expects the number to be printed stored in the accumulator.
remain:  .EQUATE 0           ;#2d Remainder of value to output
outYet:  .EQUATE 2           ;#2d Has a character been output yet?
place:   .EQUATE 4           ;#2d Place value for division
toPrint: .EQUATE 8           ;#2d Number to be printed
decPrint:SUBSP   6,i         ;Allocate @locals #remain#outYet#place
         LDWA    toPrint,s   ;Load the number to print
         CPWA    0,i         ;If oprnd is negative then
         BRGE    printMag
         LDBX    '-',i       ;Print leading '-'
         STBX    charOut,d
         NEGA                ;Make magnitude positive
printMag:STWA    remain,s    ;remain <- abs(oprnd)
         LDWA    0,i         ;Initialize outYet <- false
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
         LDWA    1,i         ;outYet <- true
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
;******* FORTH words: stack manipulation
@DC      HALT, 0x0000, 0x04, 0x09
HALT:    LDWA    0xDEAD, i
         STBA    pwrOff, d
hang:    BR      hang

@DC      DROP, _HALT, 0x04, 0x04
DROP:    @POP
         RET

;Also tried implementing with XOR-swap, but that used 3 more bytes.
@DC      SWAP, _DROP, 0x04, 0x13
SWAP:    LDWA    0,x         ;Load TOS+0, store to TOS-1
         STWA    -2,x
         LDWA    2,x         ;Load TOS+1, store to TOS+0
         STWA    0,x
         LDWA    -2,x        ;Load TOS-1, store to TOS+1
         STWA    2,x
         RET

@DC      DUP, _SWAP, 0x03, 0x0A
DUP:     LDWA    0,x         ;Load TOS+0, store to TOS-1
         STWA    -2,x
         SUBX    2,i         ;Decrement PSP
         RET

@DC      OVER, _DUP, 0x04, 0x0A
OVER:    LDWA    2,x         ;Load TOS+1, store to TOS-1
         STWA    -2,x
         SUBX    2,i         ;Decrement PSP
         RET

@DC      ROT, _OVER, 0x03, 0x1C
ROT:     LDWA    0,x         ;Load TOS+0, store to TOS-1
         STWA    -2,x
         LDWA    2,x         ;Load TOS+1, store to TOS+0
         STWA    0,x
         LDWA    4,x         ;Load TOS+2, store to TOS+1
         STWA    2,x
         LDWA    -2,x        ;Load TOS-1, store to TOS+2
         STWA    4,x
         RET
;Ignoring -ROT for now since I don't (yet) need it.
@DCSTR   "2DROP\x00", DROP2, _ROT, 0x05, 0x05
DROP2:   ADDX    4,i         ;Drop top two elements of parameter stack
         RET

@DCSTR   "2DUP\x00", DUP2, _DROP2, 0x04, 0x0A
DUP2:    LDWA    0,x         ;Load TOS+0, store to TOS-2
         STWA    -4,x
         LDWA    2,x         ;Load TOS+1, store to TOS-1
         STWA    -2,x
         SUBX    4,i         ;Decrement PSP
         RET

@DCSTR   "?DUP\x00", MDUP, _DUP2, 0x04, 0x07
MDUP:    LDWA    0,x         ;Load TOS+0
         BRNE    DUP         ;If non-0, DUP
         RET
;
;******* FORTH words: arithmetic & logic
@DCSTR   "1+\x00", INCR, _MDUP, 0x02, 0x0A
INCR:    LDWA    0,x         ;Increment TOS by 1
         ADDA    1,i
         STWA    0,x
         RET

@DCSTR   "1-\x00", DECR, _INCR, 0x02, 0x0A
DECR:    LDWA    0,x         ;Decrement TOS by 1
         SUBA    1,i
         STWA    0,x
         RET

@DCSTR   "+\x00", ADD, _DECR, 0x01, 0x0D
ADD:     LDWA    2,x         ;Add TOS to TOS-1
         ADDA    0,x
         STWA    2,x
         ADDX    2,i
         RET

@DCSTR   "-\x00", SUB, _ADD, 0x01, 0x0D
SUB:     LDWA    2,x         ;Sub TOS from TOS-1
         SUBA    0,x
         STWA    2,x
         ADDX    2,i
         RET

@DC      AND, _SUB, 0x03, 0x0D
AND:     LDWA    2,x         ;Bitwise AND TOS and TOS-1
         ANDA    0,x
         STWA    2,x
         ADDX    2,i
         RET

@DC      OR, _AND, 0x02, 0x0D
OR:      LDWA    2,x         ;Bitwise OR TOS and TOS-1
         ORA     0,x
         STWA    2,x
         ADDX    2,i
         RET

@DC      XOR, _OR, 0x03, 0x0D
XOR:     LDWA    2,x         ;Bitwise XOR TOS and TOS-1
         XORA    0,x
         STWA    2,x
         ADDX    2,i
         RET

@DCSTR   "INVERT\x00", INV, _XOR, 0x06, 0x08
INV:     LDWA    0,x          ;Bitwise NOT TOS
         NOTA
         STWA    0,x
         RET
;
;******* FORTH words: memory operations
@DCSTR   "@\x00", LOAD, _INV, 0x01, 0x10
         STWX    PSP,d        ;Store PSP to global data
         LDWX    0,x          ;Get the word pointed to by PSP
         LDWA    0,x          ;Dereference that word
         LDWX    PSP,d        ;Restore PSP
         STWA    0,x          ;Store the word to TOS
         RET
@DCSTR   "!\x00", STORE, _LOAD, 0x01, 0x10
         LDWA    2,x          ;Load TOS-1 into A
         STWX    PSP,d        ;Store PSP to global data
         LDWX    0,x          ;Get the word pointed to by PSP
         STWA    0,x          ;Store TOS-1
         LDWX    PSP,d        ;Restore PSP
         RET
;
;******* FORTH words: global variables
@DVAR    STATE,  _STORE,  0x05
@DVAR    LATEST, _STATE,  0x06
@DVAR    HERE,   _LATEST, 0x04
;
;******* FORTH words: global constants
@DCONST  F_IMM,   _HERE,   0x05, 0x80
@DCONST  F_HID,   _F_IMM,  0x05, 0x20
@DCONST  F_LNMSK, _F_HID,  0x07, 0x1f
;
;******* FORTH words: standard IO
         ;( -- c )
@DC      KEY,    _F_LNMSK, 0x03, 0x0A
         LDBA    charIn,d     ;Load char from STDIN
         STBA    -1,x         ;Push to TOS,
         ADDX    1,i
         RET

         ; ( c -- )
@DC      EMIT, _KEY, 0x04, 0x0A
EMIT:    LDBA    0,x          ;Pop TOS byte into A
         ADDX    1,i          ;Increment PSP
         STBA    charOut,d    ;Print it
         RET

         ; ( -- )
@DC      CR, _EMIT, 0x02, 0x07
CR:      LDBA '\n',i
         STBA charOut,d
         RET

        ; ( &str -- )
@DCSTR  "PRINTCSTR\x00", prntCStr, _CR, 0x09,0x14
prntCStr:STWX   PSP,d
         LDWX   0,x
bPrntLp: LDBA   0,x
         BREQ   ePrntLp
         STBA   charOut,d
         ADDX   1,i
         BR     bPrntLp
ePrntLp: LDWX   PSP,d
         ADDX   2,i           ;Pop pointer
         RET

         ; ( -- len &str )
@DC      WORD, _prntCStr, 0x04, 0x43
WORD:    SUBX    2,i          ;Allocate 2 bytes for WORD length, so we can use STWX PSP,n to store to it
         STWX    PSP,d        ;Preserve PSP
         LDWX    0,i          ;Initialize buffer index
bWrdLoop:LDBA    charIn,d     ;Load char from STDIN
         CPBA   '!',i         ;Anything greater than 0x21 is printable, letting us skip whitespace checks.
         BRGE   mWrdLoop      ;This is a performance-optimization for the common case.
         CPBA   ' ',i         ;Terminate if non-leading space, tab, newline.
         BREQ    eWrdLoop
         CPBA   '\n',i
         BREQ    eWrdLoop
         CPBA   '\t',i
         BREQ    eWrdLoop
mWrdLoop:STBA    _BUF,x       ;Store char to buffer, incremeting pointer.
         ADDX    1,i
         BR      bWrdLoop
eWrdLoop:CPWX    0,i          ;Consume leading whitespace when buffer is empty.
         BREQ    bWrdLoop
         STWX    PSP,N        ;Otherwise, push length to TOS
         LDWX    PSP,d        ;Restore PSP
         LDWA    _BUF,i       ;Push buffer pointer to TOS
         STWA    -2,x
         SUBX    2,i
         RET

         ; ( n -- )
@DC      DECO, _WORD, 0x04, 0x19
DECO:    LDWA    0,x          ;Pop TOS into A
         ADDX    2,i
         STWX    PSP,d        ;Preserve PSP
         STWA    -2,s         ;Push A to return stack
         SUBSP   2,i
         CALL    decPrint
         ADDSP   2,i
         LDWX    PSP,d        ;Restore PSP
         RET

         ;( -- n success.u8 )
@DC      DECI,   _DECO, 0x04, 0x25
DECI:    CALL    WORD
         ADDX    4,i          ;Drop word/length from stack.
         STWX    PSP,d        ;Preserve PSP
         SUBSP   3,i          ;@params#total#success
         LDBA    1,i          ;success <- true
         STBA    2,s
         CALL    decRead
         LDWX    PSP,d        ;Restore PSP
         LDWA    0,s          ;A<-total
         STWA    -2,x         ;TOS<-total
         LDBA    2,s          ;A<-success
         STBA    -3,x         ;TOS<-success
         SUBX    3,i
         ADDSP   3,i          ;@params#success#total
         RET
;
;******* FORTH words: dictionary access
         ;( &fEnt -- *(fEnt->link) )
@DC      PREV, _DECI, 0x04, 0x05
PREV:    LDWA    0,x
         STWA    -2,s
         LDWA    -2,sf
         STWA    0,x
         RET

         ; ( len &str -- fEnt)
fEnt:    .EQUATE 9            ;#2h Address of start of dictionary entry
fEq:     .EQUATE 8            ;#1c Equal boolean
fWLen:   .EQUATE 6            ;#2d Length of scanned word
fWPtr:   .EQUATE 4            ;#2h Address of start of scanned word
fELen:   .EQUATE 2            ;#2d Length of entry string
fEPtr:   .EQUATE 0            ;#2h Address of start of entry string
@DC      FIND, _PREV, 0x04, 0x65
FIND:    SUBSP   11,i         ;@locals#fEnt#fWLen#fWPtr#fELen#fEPtr#fEq
         LDWA    0,x          ;fWPtr <- &_buf
         STWA    fWPtr,s
         LDWA    2,x          ;fWLen <- len(&_buf)
         STWA    fWLen,s
         ADDX    2,i          ;_ <- POP()
         STWX    PSP,d        ;PSP <- X
         LDWX    LATEST,d     ;fEnt <- LATEST
         STWX    fEnt,s
;
         ;Assumes X<-fEnt
fndDo:   LDBA    2,x          ;A <- (fEnt->strLen & LEN_MASK)
         ANDA    F_LNMSK,i
         CPWA    fWLen,s      ;Length mismatch?
         BRNE    fNext        ;  Try next word
;
         STWA    fELen,s      ;fELen <- (fEnt->strlen & LEN_MASK)
         ADDA    1,i          ;Account for null terminator
         NEGA
         STWX    fEPtr,s      ;fEPtr <- &fEnt
         ADDA    fEPtr,s      ;fEPtr <- &fEnt - length -1
         STWA    fEPtr,s
         LDBA    -1,i
         STBA    fEq,s        ;fEq <- not equal
         CALL    strCmp       ;Compare strings
         LDBA    fEq,s
         BRNE    fNext        ;If strings don't match, try next word
         BR      fEnd         ;fEnt matches input string; stop and push to PSP.
;
fNext:   LDWX    fEnt,sf      ;fEnt <- fEnt->link
         STWX    fEnt,s
         BREQ    fEnd         ;End iteration if fEnt == 0
         BR      fndDo
;
fEnd:    LDWA    fEnt,s
         LDWX    PSP,d        ;X <- PSP
         STWA    0,x          ;PUSH(fEnt)
         ADDSP   11,i         ;@locals#fEq#fEPtr#fELen#fWPtr#fWLen#fEnt
         RET

;        ( &fEnt -- &fEnt->code )
@DCSTR   ">CFA\x00", CFA, _FIND, 0x04, 0x0A
         LDWA    0,x          ;Code address is 3 bytes from start of link ptr
         ADDA    3,i
         STWA    0,x
         RET

        ;( &fEnt -- &fEnt->str )
@DCSTR   ">STR\x00", STR, _CFA, 0x04, 0x17
derefStr:LDWA    0,x          ;A <- &(fEnt->len)
         ADDA    2,i
         STWA    -2,s         ;A <- fEnt->len
         LDBA    -2,sf
         ADDA    1,i          ;Account for null terminator
         ANDA    F_LNMSK,i    ;A <- -(MASK && fEnt->len)
         NEGA
         ADDA    0,x          ;A <- &(fEnt->str)
         STWA    0,x
         RET

         ;( -- )
@DCSTR   "DUMPDICT\x00", DD, _STR, 0x08, 0x1c
DD:      LDWA    LATEST,d
         SUBX    2,i
         STWA    0,x
_ddLoop: CALL    DUP
         CALL    derefStr
         CALL    prntCStr
         CALL    CR
         CALL    PREV
         BRNE    _ddLoop
         ADDX   2,i
         RET
;
;******* FORTH words: basic compilation
         ;( len &str -- )
@DC      CREATE, _DD, 0x06, 0x6A
;        Copy string to HERE++
CREATE:  STWX    PSP,d        ;*PSP <- X
         SUBSP   3,i          ;@params#2h#1d
         LDWA    0,x          ;A <- str
         STWA    0,s
         LDBA    2,x          ;A <- len
         STBA    2,s
         ;
         LDBX    0,i
bCrLoop: CPBX    2,s
         BRGE    eCrLoop
         ADDX    1,i
         LDBA    0,sfx        ;**here <- str[x]
         STBA    HERE,n       ;*here <- *here +1
         LDWA    HERE,d
         ADDA    1,i
         STWA    HERE,d
         BR      bCrLoop
eCrLoop: ADDSP   3,i          ;@locals#2h#1d
         LDWX    PSP,d
;
                              ;Copy value of LATEST to *HERE
         LDWA    LATEST,d     ; **HERE <- *LATEST
         STWA    HERE,n
         LDWA    HERE,d       ;*LATEST <- *HERE
         STWA    LATEST,d
         ADDA    2,i          ;*HERE <- *HERE + 2
         STWA    HERE,d
;
                              ;Copy length of string as u8
         LDBA    2,x          ;**HERE <- len
         STBA    HERE,n
         LDWA    HERE,d       ;*HERE <- *HERE + 1
         ADDA    1,i
         STWA    HERE,d
                              ;Set code len to 0
         LDWA    0,i          ;**HERE <- 0
         STWA    HERE,n
         LDWA    HERE,d       ;*HERE <- *HERE + 1
         ADDA    1,i
         STWA    HERE,d
         SUBX    3,i          ;POP(len, *str)
         RET

         ;( n -- )
@DCSTR   ",\x00", COMMA, _CREATE, 0x01, 0x13
COMMA:   @POPA                ;A <- TOS
         STWA    HERE,n       ;**HERE <- A
         LDWA    HERE,d       ;*HERE += 2
         ADDA    2,i
         STWA    HERE,d
         RET

         ;( -- )
@DCSTR   "CALL,\x00", CCOMMA, _COMMA, 0x05, 0x13
         LDBA    __call,d     ;**HERE <- opcode(CALL)
         STBA    HERE,n
         LDWA    HERE,d       ;*HERE += 1
         ADDA    1,i
         STWA    HERE,d
__call:  CALL    COMMA
         RET

         ; ( -- )
@DCSTR   "[\x00", LBRAC, _CCOMMA, 0x81, 0x07
LBRAC:   LDWA    0,i          ;STATE <- 0
         STWA    STATE,d
         RET

         ; ( -- )
@DCSTR   "]\x00", RBRAC, _LBRAC, 0x01, 0x07
RBRAC:   LDWA    1,i          ;STATE <- 1
         STWA    STATE,d
         RET

         ; ( -- )
@DCSTR   "IMMEDIATE\x00",IMM,_RBRAC,0x89,0x13
         LDWA    LATEST,d
         ADDA    1,i
         STWA    -2,s
         LDBA    -2,sf
         XORA    F_IMM,i
         STWA    -2, sf
         RET

         ; ( -- )
@DCSTR   "HIDDEN\x00",HIDDEN,_IMM,0x06,0x13
HIDDEN:  LDWA    LATEST,d
         ADDA    1,i
         STWA    -2,s
         LDBA    -2,sf
         XORA    F_HID,i
         STWA    -2, sf
         RET

         ; ( -- )
@DCSTR   ":\x00", COLON, _HIDDEN, 0x01, 0x0D
         CALL    WORD
         CALL    CREATE
         CALL    HIDDEN
         CALL    RBRAC
         RET

         ; ( -- )
@DCSTR   ";\x00", SEMI, _COLON, 0x81, 0x1c
         LDBA    __ret,d      ;**HERE <- opcode(RET)
         STBA    HERE,n
         LDWA    HERE,d       ;*HERE += 2
         ADDA    2,i
         STWA    HERE,d
         CALL    HIDDEN
         CALL    LBRAC
__ret:   RET
;
;******* FORTH words: control flow
;
;******* FORTH words: core interpreter
cldstrt: LDWX    pStack, i
         CALL    DECI
         ADDX    1,i         ;Drop flag
         CALL    DECI
         ADDX    1,i         ;Drop flag
         CALL    ADD
         CALL    DECO
         CALL    HALT
;
         .SECTION "memvec", "rw"
;
;******* FORTH Globals
         .BLOCK  2           ;Padding
PSP:     .WORD   pStack      ;Current parameter stack pointer
RSP:     .WORD   rStack      ;Current return stack pointer
STATE:   .WORD   0           ;0=interpret, !0=compile
LATEST:  .WORD   _SEMI       ;Pointer to the most recently defined word
HERE:    .WORD   0x0000      ;Pointer to the next free memory location
; Probably should be RO, but I don't want to add another section.
;While bare metal mode is not supposed to have a trap handler,
;failure to provide one could cause user-programs to enter an infinite loop.
;This trap handler prints no message to save ~60B.
trpHnd:  .WORD   HALT        ;Address of first instruction in trap handler.
initPC:  .WORD   cldstrt     ;Address of first instruction to execute on boot.
         .ORG    0xFFFB
initSP:  .WORD   rStack
         .EXPORT charIn
         .INPUT  charIn
charIn:  .BLOCK  1
         .EXPORT charOut
         .OUTPUT charOut
charOut: .BLOCK  1
         .EXPORT pwrOff
         .OUTPUT pwrOff
pwrOff:  .BLOCK  1
