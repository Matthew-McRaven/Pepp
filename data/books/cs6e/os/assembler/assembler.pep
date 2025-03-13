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
inSuc:   .EQUATE 12          ;#1d Success boolean
inBase:  .EQUATE 10          ;#2d Base in which INTI works
inTotal: .EQUATE 8           ;#2d Cumulative total of DECI number
inIdx:   .EQUATE 4           ;#2d Index into _BUF
inState: .EQUATE 3           ;#1d State variable
inASCII: .EQUATE 2           ;#1c asciiCh, one byte
inInt:   .EQUATE 1           ;#1d ascii as integer, one byte
inNeg:   .EQUATE 0           ;#1d Negative boolean
;States
inSInit: .EQUATE 0           ;Enumerated values for state
inSSign: .EQUATE 1
inSDigit:.EQUATE 2
;
inti:    SUBSP   6,i         ;@locals#inIdx#inState#inASCII#inInt#inNeg
         LDWA    0,i
         STWA    inTotal,s   ;inTotal <- 0
         STWA    inIdx,s     ;inIdx <- 0
         STBA    inState,s   ;inState <- false
         STBA    inASCII,s   ;inASCII <- 0
         STBA    inInt,s     ;inInt <- 0
         STBA    inNeg,s     ;inNeg <- 0
;
inSLoop: LDWX    inIdx,s
         LDBA    _BUF,x
         ADDX    1,i
         STWX    inIdx,s
         LDBX    inState,s
         ASLX
         BR      inJT,x
inJT:    .WORD   inInit
         .WORD   inDigit
         .WORD   inDigit
;
inInit:  CPBA    '+',i
         BRNE    inMinus
         ;inNeg defaulted to 0.
         LDWX    inSSign,i
         STBX    inState,s
         BR      inSLoop
inMinus: CPBA    '-',i
         BRNE    inDigit
         LDWX    1,i
         STBX    inNeg,s
         LDWX    inSSign,i
         STBX    inState,s
         BR      inSLoop
;
inDigit: CPBA    '0',i       ;else if (asciiCh is a digit)
         BRLT    inWhite
         CPBA    '9',i
         BRGT    inChar
         SUBA    '0',i
         STBA    inInt,s
         BR      inComb
;
inChar:  ANDA    0xDF,i      ;Convert to uppercase
         CPBA    'A',i       ;else if (asciiCh is a digit)
         BRLT    inWhite
         CPBA    'Z',i
         BRGT    inErr
         SUBA    0x37,i       ;'A'-0x37 == 0xA
         STBA    inInt,s
         BR      inComb
;
inWhite: LDBX    inState,s   ;If (state == sign) goto inErr
         CPBX    inSSign,i
         BREQ    inErr
         CPBA    ' ',i       ;else if (asciiCh is not a space
         BREQ    inRep
         CPBA    '\n',i      ;or line feed
         BREQ    inRep
         CPBA    0,i         ;or null terminator)
         BRNE    inErr       ;exit with error
         BR      inRep
;
inRep:   LDBX    inState,s   ;If (state == digit) goto end
         CPBX    inSDigit,i
         BREQ    inSign
         BR      inSLoop
;
inComb:  LDWX    inSDigit,i
         STBX    inState,s
         CPWA    inBase,s
         BRGT    inErr

         LDWA    inBase,s   ;mN1Lo <- inBase
         STWA    -8,s
         LDWA    inTotal,s  ;mN2Hi <- inTotal
         STWA    -6,s
         SUBSP   8,i
         CALL    mul
         ADDSP   8,i

         LDBA    inInt,s
         ADDA    -2,s       ;inTotal <- inTotal + mProdLo
         STWA    inTotal,s
         BR      inSLoop

inErr:   LDBA    0,i
         STBA    inSuc,s
inSign:  LDBA    inNeg,s
         BREQ    inRet
         LDWA    inTotal,s
         NEGA
         STWA    inTotal,s
inRet:   ADDSP   6,i
         RET
;
othTotal:.EQUATE 6          ;#2h Value to output
othBase: .EQUATE 4          ;#2d Base in which to output
othBaseN:.EQUATE 2          ;#2d Base ^ N
intoh:   LDWA    othBase,s  ;Compute base*base^N
         LDWX    othBaseN,s
         SUBSP   8,i
         STWA    0,s
         STWX    2,s
         CALL    mul
         LDWX    6,s        ;X <- mProdLo
         LDWA    4,s        ;A <- mProdHi
         ADDSP   8,i
         BRNE    othDiv     ;If base^N+1 oveflows 16-bits, don't recurse
         STWX    -6,s       ;othBaseN (theirs) <- X. If we don't branch to othRec, it is a dead store.
         LDWX    othTotal,s ;if (othTotal < 0), use unsigned comparison
         BRLT    othNeg
         CPWX    -6,s       ;if (othTotal > base^N), don't recurse
         BRLT    othDiv
         BR      othRec
othNeg:  CPWX    -6,s       ;if (unsigned(othTotal) > unsigned(base^N)), don't recurse
         BRGT    othDiv
othRec:  LDWA    othBase,s  ;othBase (theirs) <- othBase  (ours)
         STWA    -4,s
         LDWA    othTotal,s ;othTotal (theirs) <- othTotal (ours)
         STWA    -2,s
         SUBSP   6,i        ;@params#othTotal#othBase#othBaseN
         CALL    intoh
         ADDSP   6,i        ;@params#othBaseN#othBase#othTotal
         LDWA    -2,s       ;othTotal (ours) <- othTotal (theirs). Their total contains remainder of othTotal (ours) / othBase^(N+1)
         STWA    othTotal,s
;
othDiv:  LDWA    othTotal,s
         LDWX    othBaseN,s
         SUBSP   8,i        ;@params#dQ#dR#dZ#dD
         STWA    2,s        ;Z <- othTotal
         STWX    0,s        ;D <- base^N
         CALL    div
         LDWX    4,s        ;X <- remainder
         LDWA    6,s        ;A <- quotient
         ADDSP   8,i
         STWX    othTotal,s
         CPBA    9,i
         BRGT    othChar
         ADDA    '0',i
         BR      othPrnt
othChar: ADDA    0x37,i     ; 'A' - 10
othPrnt: STBA    charOut,d
         RET
;
;Output format: If the operand is negative, the algorithm prints
;a single '-' followed by the magnitude. Otherwise it prints the
;magnitude without a leading '+'. It suppresses leading zeros.
otTotal: .EQUATE 5           ;#2h Value to output
otBase:  .EQUATE 3           ;#2d Base in which to output
otSigned:.EQUATE 2           ;#1d Treat the number as signed?
into:    LDWA    otTotal,s   ;if(otTotal < 0
         BRGE    otCall
         LDBX    otSigned,s  ;  && otSigned)
         BREQ    otCall
         LDBX    '-',i
         STBX    charOut,d
         NEGA
         STWA    otTotal,s
;
otCall:  LDWX    otBase,s    ;X <- otBase
         SUBSP   6,i         ;@params#othTotal#othBase#othBaseN
         STWA    4,s         ;othTotal <- otTotal
         STWX    2,s         ;othBase  <- otBase
         LDWX    1,i         ;othBaseN <- 1
         STWX    0,s
         CALL    intoh
         ADDSP   6,i         ;@params#othBaseN#othBase#othTotal
         RET
;
;Subroutine to multiply two 16-bit integers together, returning the product.
;It probably misbehaves if either input is negative, and overflows
;if the product is greater than 0xFFFF.
mProdLo: .EQUATE 10         ;#2h Product of mN2 * mN1
mProdHi: .EQUATE 8          ;#2h Product of mN2 * mN1
mN1Lo:   .EQUATE 6          ;#2h Integer larger than mN2
mN2:     .EQUATE 4          ;#2h Integer smaller than mN1
mN1Hi:   .EQUATE 0          ;#2h Integer larger than mN2
mul:     SUBSP   2,i        ;@locals#mN1Hi
         LDWA    0,i
         STWA    mProdLo,s
         STWA    mProdHi,s
         STWA    mN1Hi,s
         LDWA    mN1Lo,s
         CPWA    mN2,s
         BRGE    mBLoop     ;If mN1 < mN2, swap
         LDWX    mN2,s
         STWA    mN2,s
         STWX    mN1Lo,s
;
mBLoop:  LDWX    mN2,s      ;Multiply setup loop
         CPWX    0,i
         BREQ    mELoop
         ANDX    1,i
         BREQ    mCLoop
         LDWA    mProdLo,s  ;mProd <- mProd + mN1
         LDWX    mProdHi,s
         ADDA    mN1Lo,s
         BRC     mCHi
mALoop:  ADDX    mN1Hi,s    ;Multiple add in loop
         STWA    mProdLo,s  ;Write back mProd
         STWX    mProdHi,s
;
mCLoop:  LDWX    mN2,s      ;mN2 <- mN2/2
         RORX
         CPWA    0,i        ;C is not cleared by load. N-0 should never carry out.
         STWX    mN2,s
         LDWX    mN1Hi,s    ;mN1 <- mN1*2
         LDWA    mN1Lo,s
         ROLA
         ROLX               ;C is set by shift
         STWA    mN1Lo,s
         STWX    mN1Hi,s
         BR      mBLoop
;
mELoop:  ADDSP   2,i        ;@locals#mN1Hi
         RET
;
mCHi:    ADDX    1,i
         BR      mALoop
;
;Subroutine to divide two 16-bit integers, the dividend (z) and divisod (d).
;It returns both the quotient (q) and remainder (r).
dQ:      .EQUATE 10          ;#2h Quotient of z/d
dR:      .EQUATE 8           ;#2h Remainder of z/d
dZ:      .EQUATE 6           ;#2h Dividend
dD:      .EQUATE 4           ;#2h Divisor
dM:      .EQUATE 0           ;#2h Bitmask for division
div:     SUBSP   2,i         ;@locals#dM
         LDWA    0x8000,i    ;dM <- 0x8000
         STWA    dM,s        ;  We could identify leading digit in D to reduce iterations.
         LDWA    0,i
         CPWA    dD,s        ;Error is divisor==0
         BREQ    divErr
         STWA    dQ,s        ;q <- 0
         STWA    dR,s        ;r <- 0
;
;Prepare loop invariant A<-dR
         LDWA    dR,s
;R remains resident in A through whole loop.
divLoop: ASLA                 ;r <- r << 1
         LDWX    dZ,s        ;r <- r | (dividend & mask ? 1 : 0)
         ANDX    dM,s
         BREQ    divLPQ
         ORA     1,i
divLPQ:  LDWX    dQ,s        ;q <- q << 1
         ASLX
         CPWA    dD,s        ;if (r >= abs(d))
         BRLT    divLPWB     ;div loop writeback
         SUBA    dD,s        ;  r <- r - abs(d)
         ORX     1,i         ;  q <- q | 1
divLPWB: STWX    dQ,s
         LDWX    dM,s        ;m <- m >> 1
         CPWA    0,i         ;Clear C
         RORX
         STWX    dM,s
         CPWX    0,i         ;Ensure X is non-zero
         BRNE    divLoop     ;Repeat while non-0 mask
;
divRWB:  STWA    dR,s        ;Write back r, freeing A
divRet:  ADDSP   2,i         ;@locals#dM
         RET
divErr:  CALL    HALT
;Subroutine to divide two 16-bit integers, the dividend (z) and divisod (d).
;It returns both the quotient (q) and remainder (r).
sdQ:     .EQUATE 9           ;#2h Quotient of z/d
sdR:     .EQUATE 7           ;#2h Remainder of z/d
sdZ:     .EQUATE 5           ;#2h Dividend
sdD:     .EQUATE 3           ;#2h Divisor
sdSign:  .EQUATE 0           ;#1d Sign of dividend
sdAbsQ:  .EQUATE -2          ;#2h Absolute value of quotient
sdAbsR:  .EQUATE -4          ;#2h Absolute value of remainder
sdAbsZ:  .EQUATE -6          ;#2h Absolute value of dividend
sdAbsD:  .EQUATE -8          ;#2h Absolute value of divisor
sdiv:    SUBSP   1,i         ;@locals#sdSign
         LDWX    sdZ,s       ;sdSign <- sign(z) XOR sign(d)
         XORX    sdD,s
         ROLX
         ROLX
         ANDX    1,i
         STBX    sdSign,s
; Compute abs(dZ)
         LDWA    sdZ,s
         BRGE    sdNorm
         NEGA
sdNorm:  STWA    sdAbsZ,s
;Compute abs(dD)
         LDWA    sdD,s
         BRGE    sdNorm2
         NEGA
sdNorm2: STWA    sdAbsD,s
         SUBSP   8,i         ;@params#dQ#dR#dZ#dD
         CALL    div
         ADDSP   8,i         ;@params#dD#dZ#dR#dQ
         LDWA    sdAbsR,s
         LDWX    sdZ,s
         BRGE    sdRWB       ;if (dividend <0)
         NEGA                ;  r <- -r
sdRWB:   STWA    sdR,s       ;Write back r, freeing A
         LDWA    sdAbsQ,s
         LDBX    sdSign,s
         BREQ    sdQWB
         NEGA
sdQWB:   STWA    sdQ,s
sdRet:   ADDSP   1,i         ;@locals#sdSign
         RET
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

         ;( n1 -- n1 n1)
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

         ;( n1 n2 n3 -- n2 n1 n3)
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

         ;( n1 n2 -- )
@DCSTR   "2DROP\0", DROP2, _ROT, 0x05, 0x05
DROP2:   ADDX    4,i         ;Drop top two elements of parameter stack
         RET

         ;( n1 n2 -- n2 n1 n2 n1)
@DCSTR   "2DUP\0", DUP2, _DROP2, 0x04, 0x0A
DUP2:    LDWA    0,x         ;Load TOS+0, store to TOS-2
         STWA    -4,x
         LDWA    2,x         ;Load TOS+1, store to TOS-1
         STWA    -2,x
         SUBX    4,i         ;Decrement PSP
         RET

@DCSTR   "?DUP\0", MDUP, _DUP2, 0x04, 0x07
MDUP:    LDWA    0,x         ;Load TOS+0
         BRNE    DUP         ;If non-0, DUP
         RET
;
;******* FORTH words: arithmetic & logic
         ;( i8 -- i16), sign extend a byte to a word
@DCSTR   "SE\0", SE, _MDUP, 0x02, 0x13
         LDBA    0,x
         ANDX    0x80,i
         BREQ    seNotN
         ORX     0xFF00,i
seNotN:  SUBX    1,i
         STWA    0,x
         RET


@DCSTR   "1+\0", INCR, _SE, 0x02, 0x0A
INCR:    LDWA    0,x         ;Increment TOS by 1
         ADDA    1,i
         STWA    0,x
         RET

@DCSTR   "1-\0", DECR, _INCR, 0x02, 0x0A
DECR:    LDWA    0,x         ;Decrement TOS by 1
         SUBA    1,i
         STWA    0,x
         RET

         ;( n1 n2 -- sum)
@DCSTR   "+\0", ADD, _DECR, 0x01, 0x0D
ADD:     LDWA    2,x         ;Add TOS to TOS-1
         ADDA    0,x
         STWA    2,x
         ADDX    2,i
         RET

         ;( n1 n2 -- n1-n2)
@DCSTR   "-\0", SUB, _ADD, 0x01, 0x0D
SUB:     LDWA    2,x         ;Sub TOS from TOS-1
         SUBA    0,x
         STWA    2,x
         ADDX    2,i
         RET

         ;( n1 n2 -- product[hi] product[lo] )
@DCSTR   "*\0", MUL, _SUB, 0x01, 0x00
MUL:     LDWA    2,x         ;Multiple TOS and TOS+1
         SUBSP   8,i
         STWA    0,s
         LDWA    0,x
         STWA    2,s
         STWX    PSP,d
         CALL    mul
         LDWX    PSP,d
         ADDSP   8,i
         LDWA    -2,s
         STWA    2,x
         LDWA    -4,s
         STWA    0,x
         RET

         ;(divisor dividend -- remainder quotient)
@DCSTR   "/mod\0", DIVMOD, _MUL, 0x04, 0x00
DIVMOD:  LDWA    2,x         ;Divide TOS by TOS-1
         SUBSP   8,i
         STWA    0,s
         LDWA    0,x
         STWA    2,s
         STWX    PSP,d
         CALL    sdiv
         LDWX    PSP,d
         LDWA    6,s         ;Quotient
         STWA    2,x
         LDWA    4,s         ;Remainder
         STWA    0,x
         ADDSP   8,i
         RET

         ;(divisor dividend -- remainder)
@DCSTR   "%\0", MOD, _DIVMOD, 0x01, 0x0D
         CALL    DIVMOD
         LDWA    0,x
         STWA    2,x
         ADDX    2,i
         RET

         ;(divisor dividend -- quotient)
@DCSTR   "/\0", DIV, _MOD, 0x01, 0x07
         CALL    DIVMOD
         ADDX    2,i
         RET

         ;(n1 n2 -- n1 & n2)
@DC      AND, _DIV, 0x03, 0x0D
AND:     LDWA    2,x         ;Bitwise AND TOS and TOS-1
         ANDA    0,x
         STWA    2,x
         ADDX    2,i
         RET

         ;(n1 n2 -- n1 | n2)
@DC      OR, _AND, 0x02, 0x0D
OR:      LDWA    2,x         ;Bitwise OR TOS and TOS-1
         ORA     0,x
         STWA    2,x
         ADDX    2,i
         RET

         ;(n1 n2 -- n1 ^ n2)
@DC      XOR, _OR, 0x03, 0x0D
XOR:     LDWA    2,x         ;Bitwise XOR TOS and TOS-1
         XORA    0,x
         STWA    2,x
         ADDX    2,i
         RET

         ;( n -- -1*n)
@DCSTR   "INVERT\0", INV, _XOR, 0x06, 0x08
INV:     LDWA    0,x          ;Bitwise NOT TOS
         NOTA
         STWA    0,x
         RET
;
;******* FORTH words: comparisons
         ;( n1 n2 -- n1 == n2)
@DCSTR   "=\0",EQ, _INV, 0x01, 0x19
         LDWA    2,x         ;Compare TOS and TOS-1
         CPWA    0,x
         BREQ    eqTrue
         LDWA    0,i
         BR      eqPop
eqTrue:  LDWA    0xffff,i
eqPop:   ADDX    2,i
eqStore: STWA    0,x
         RET

         ;( n1 -- n ==0)
@DCSTR   "0=\0",EQ0, _EQ, 0x02, 0x00
         LDWA    2,x         ;Compare TOS to 0
         CPWA    0,i
         BREQ    eq0True
         LDWA    0,i
         BR      eqStore
eq0True: LDWA    0xffff,i
         BR      eqStore

;
;******* FORTH words: memory operations
@DCSTR   "@\0", LOAD, _EQ0, 0x01, 0x10
         STWX    PSP,d        ;Store PSP to global data
         LDWX    0,x          ;Get the word pointed to by PSP
         LDWA    0,x          ;Dereference that word
         LDWX    PSP,d        ;Restore PSP
         STWA    0,x          ;Store the word to TOS
         RET
@DCSTR   "!\0", STORE, _LOAD, 0x01, 0x10
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
@DCONST  F_IMM,   _HERE,    0x05, 0x80
@DCONST  F_HID,   _F_IMM,   0x05, 0x20
@DCONST  F_LNMSK, _F_HID,   0x07, 0x1f
@DCSTR   "'('\0" , lParen, _F_LNMSK, 0x03, 0x00
         LDBA    '(',i
pushba:  SUBX    1,i
         STBA    0,x
         RET

@DCSTR   "')'\0", rParen, _lParen, 0x03, 0x00
         LDBA    ')',i
         BR      pushba
;
;******* FORTH words: standard IO
         ;( -- c )
@DC      KEY,    _rParen, 0x03, 0x0A
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
@DCSTR  "PRINTCSTR\0", prntCStr, _CR, 0x09,0x14
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
@DC      WORD, _prntCStr, 0x04, 0x49
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
         LDBA    0,i          ;Ensure BUF is null terminated
         STBA    _BUF,x
         LDWX    PSP,d        ;Restore PSP
         LDWA    _BUF,i       ;Push buffer pointer to TOS
         STWA    -2,x
         SUBX    2,i
         RET

         ;( n -- )
@DCSTR   ".\0", DECO, _WORD, 0x01, 0x19
DECO:    LDWA    10,i         ;Load base into A
         SUBSP   5,i          ;@params#otTotal#otBase#otSign
         STWA    1,s          ;otBase <- 10
         STBA    0,s          ;otSign <- true
         LDWA    0,x          ;Pop TOS into A
         ADDX    2,i
         STWX    PSP,d        ;Preserve PSP
         STWA    3,s          ;otTotal <- TOS
         CALL    into
         ADDSP   5,i          ;@params#otSign#otBase#otTotal
         LDWX    PSP,d        ;Restore PSP
         RET
;
         ;(n -- )
@DCSTR   "HEX.\0", HEXO, _DECO, 0x04, 0x19
HEXO:    LDWA    16,i         ;Load base into A
         SUBSP   5,i          ;@params#otTotal#otBase#otSign
         STWA    1,s          ;otBase <- 10
         LDBA    0,i
         STBA    0,s          ;otSign <- false
         LDWA    0,x          ;Pop TOS into A
         ADDX    2,i
         STWX    PSP,d        ;Preserve PSP
         STWA    3,s          ;otTotal <- TOS
         CALL    into
         ADDSP   5,i          ;@params#otSign#otBase#otTotal
         LDWX    PSP,d        ;Restore PSP
         RET

         ;( -- n success.u8 )
@DC      DECI,   _HEXO, 0x04, 0x25
DECI:    CALL    WORD
         ADDX    4,i          ;Drop word/length from stack.
DECICore:STWX    PSP,d        ;Preserve PSP
         SUBSP   5,i          ;@params#inSuc#inTotal#inBase
         LDBA    1,i          ;success <- true
         STBA    4,s
         LDWA    10,i         ;base <- 10
         STWA    2,s
         CALL    inti
         LDWX    PSP,d        ;Restore PSP
         LDWA    0,s          ;A<-total
         STWA    -2,x         ;TOS<-total
         LDBA    4,s          ;A<-success
         STBA    -3,x         ;TOS<-success
         SUBX    3,i
         ADDSP   5,i          ;@params#inBase#inSuc#inTotal
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

         ;( len &str -- fEnt)
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

         ;( &fEnt -- &fEnt->code )
@DCSTR   ">CFA\0", CFA, _FIND, 0x04, 0x0A
CFA:     LDWA    0,x          ;Code address is 3\4 bytes from start of link ptr
         ADDA    4,i
         STWA    0,x
         RET

        ;( &fEnt -- &fEnt->str )
@DCSTR   ">STR\0", STR, _CFA, 0x04, 0x17
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
@DCSTR   "DUMPDICT\0", DD, _STR, 0x08, 0x1c
DD:      LDWA    LATEST,d
__subxi: SUBX    2,i
__stwax: STWA    0,x
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
         LDWA    2,x          ;A <- len
         STBA    2,s
         ;
         LDBX    0,i
bCrLoop: CPBX    2,s
         BRGE    eCrLoop
         LDBA    0,sfx        ;**here <- str[x]
         STBA    HERE,n       ;*here <- *here +1
         LDWA    HERE,d
         ADDA    1,i
         STWA    HERE,d
         ADDX    1,i
         BR      bCrLoop
eCrLoop: ADDSP   3,i          ;@locals#2h#1d
;
         LDBA    0,i          ;Add null terminator for ease of printing
         STBA    HERE,n
         ADDX    1,i
         LDWA    HERE,d
         ADDA    1,i
         STWA    HERE,d
;
                              ;Copy value of LATEST to *HERE
         LDWX    PSP,d
         LDWA    LATEST,d     ; **HERE <- *LATEST
         STWA    HERE,n
         LDWA    HERE,d       ;*LATEST <- *HERE
         STWA    LATEST,d
         ADDA    2,i          ;*HERE <- *HERE + 2
         STWA    HERE,d
;
                              ;Copy length of string as u8
__ldwax: LDWA    2,x          ;**HERE <- len
         STBA    HERE,n
         LDWA    HERE,d       ;*HERE <- *HERE + 1
         ADDA    1,i
         STWA    HERE,d
                              ;Set code len to 0
__ldwai: LDWA    0,i          ;**HERE <- 0
         STBA    HERE,n
         LDWA    HERE,d       ;*HERE <- *HERE + 1
         ADDA    1,i
         STWA    HERE,d
         SUBX    3,i          ;POP(len, *str)
         RET

         ;Store byte in A to *(here++)
STBAH:   STBA    HERE,n
         LDWA    HERE,d       ;*HERE += 1
         ADDA    1,i
         STWA    HERE,d
         RET

         ;Store word in A to *(here+=2)
STWAH:   STWA    HERE,n
         LDWA    HERE,d       ;*HERE += 2
         ADDA    2,i
         STWA    HERE,d
         RET
         ;( n -- )
@DCSTR   ",\0", COMMA, _CREATE, 0x01, 0x00
COMMA:   @POPA                ;A <- TOS
         BR      STWAH


         ;( -- )
@DCSTR   "CALL,\0", CALLC, _COMMA, 0x05, 0x13
CALLC:   LDBA    __call,d     ;**HERE <- opcode(CALL)
storeOp: CALL    STBAH
__call:  CALL    COMMA
         RET

         ; ( n -- )
@DCSTR   "LDWAi,\0", LDWAC, _CALLC, 0x06, 0x00
LDWAC:   LDBA    __ldwai,d     ;**HERE <- opcode(LDWA,i)
         BR      storeOp

         ; ( n -- )
@DCSTR   "STWAx,\0", STWAXC, _LDWAC, 0x06, 0x00
STWAXC:  LDBA    __stwax,d     ;**HERE <- opcode(STWA,x)
         BR      storeOp

         ; ( n -- )
@DCSTR   "SUBXi,\0", SUBXIC, _STWAXC, 0x06, 0x13
SUBXIC:  LDBA    __subxi,d     ;**HERE <- opcode(SUBX,i)
         BR      storeOp

         ; ( -- )
@DCSTR   "[\0", LBRAC, _SUBXIC, 0x81, 0x07
LBRAC:   LDWA    0,i          ;STATE <- 0
         STWA    STATE,d
         RET

         ; ( -- )
@DCSTR   "]\0", RBRAC, _LBRAC, 0x01, 0x07
RBRAC:   LDWA    1,i          ;STATE <- 1
         STWA    STATE,d
         RET

         ; ( -- )
@DCSTR   "IMMEDIATE\0",IMM,_RBRAC,0x89,0x13
         LDWA    LATEST,d
         ADDA    2,i
         STWA    -2,s
         LDBA    -2,sf
         XORA    F_IMM,i
         STBA    -2, sf
         RET

         ; ( -- )
@DCSTR   "HIDDEN\0",HIDDEN,_IMM,0x06,0x13
HIDDEN:  LDWA    LATEST,d
         ADDA    2,i
         STWA    -2,s
         LDBA    -2,sf
         XORA    F_HID,i
         STBA    -2, sf
         RET

         ; ( -- )
@DCSTR   ":\0", COLON, _HIDDEN, 0x01, 0x0D
         CALL    WORD
         CALL    CREATE
         CALL    HIDDEN
         CALL    RBRAC
         RET

         ; ( -- )
@DCSTR   ";\0", SEMI, _COLON, 0x81, 0x1c
         LDBA    __ret,d      ;**HERE <- opcode(RET)
         STBA    HERE,n
         LDWA    HERE,d       ;*HERE += 1
         ADDA    1,i
         STWA    HERE,d
         CALL    HIDDEN
         CALL    LBRAC
__ret:   RET
;
;******* FORTH words: control flow
;
         ;Emit ADDX 2,i LDWA -2,x
emitPop: LDBA    __addxi,d    ;**HERE <- opcode(ADDX,i)
         CALL    STBAH
         LDWA    2,i
         CALL    STWAH
         ;Emit   LDWA -2,x
         LDBA    __ldwax,d    ;**HERE <- opcode(LDWA,x)
         CALL    STBAH
         LDWA    -2,i
         CALL    STWAH
         RET

emitBR:  CALL    STBAH
         LDWA    0,i
         CALL    STWAH
         RET

@DC      IF, _SEMI, 0x82, 0x00
         CALL    emitPop
         ; Emit BREQ,i with junk operand.
         LDBA    __breqi,d     ;**HERE <- opcode(BREQ,i)
         CALL    emitBR
         ;
         SUBSP   2,i
         LDWA    2,s          ;Shift return address down by 2.
         STWA    0,s
         LDWA    HERE,d
         SUBA    2,i          ;Undo writing junk operand
         STWA    2,s
         RET

@DC      ELSE, _IF, 0x84, 0x00
         ;Emit BR,iwith junk operand.
         LDBA    __bri,d    ;**HERE <- opcode(BR,i)
         CALL    STBAH
         CALL    STWAH
         ;Patch operand of previous BR.
         LDWA    HERE,d
         STWA    2,sf
         ;And mark current BR for patching
         SUBA    2,i
         STWA    2,s
         RET

@DC      THEN, _ELSE, 0x84, 0x00
         ;Patch operand of previous BR.
         LDWA    HERE,d
         STWA    2,sf
         ;Pop patch address from stack.
         LDWA    0,s
         STWA    2,s
         ADDSP   2,i
         RET

@DC      BEGIN, _THEN, 0x85, 0x00
         @PUSH  HERE,d
         RET

@DC      UNTIL, _BEGIN, 0x85, 0x00
         CALL   emitPop
         ;Emit BRNE,i with backwards branch
         LDBA   __breqi,d
         CALL   STBAH
         @POPA
         CALL   STWAH
         RET
;
;******* FORTH words: interpreter
;
@DC      INTERP, _UNTIL, 0x06, 0x00
INTERP:  CALL    WORD
         CALL    FIND
         LDWA    0,x
__brnei: BRNE    _intWord
__addxi: ADDX    2,i         ;Pop nullptr
         CALL    DECICore
         ADDX    1,i
         LDBA    -1,x
__breqi: BREQ    _intErr
         LDWA    STATE,d
         BREQ    INTERP
         CALL    LDWAC
         @PUSH   2,i
         CALL    SUBXIC
         @PUSH   0,i
         CALL    STWAXC
__bri:   BR      INTERP
_intWord:LDWA    STATE,d
         BREQ    _intImm
         LDWA    0,x         ;A <- &(fEnt->len)
         ADDA    2,i
         STWA    -2,s        ;A <- fEnt->len
         LDBA    -2,sf
         ANDA    F_IMM,i
         BRNE    _intImm
         CALL    CFA
         CALL    CALLC
         BR      INTERP
_intImm: CALL    CFA
         LDWA    INTERP,i
         STWA    -2,s
         @POPA
         STWA    -4,s
         SUBSP   4,i
         RET                 ;@call
_intErr: @PUSH   _intMsg,i
         CALL    prntCStr
         @PUSH   _BUF,i
         CALL    prntCStr
         BR      HALT
_intMsg: .ASCII "PARSE ERROR: \0"
;******* FORTH words: core interpreter
cldstrt: LDWX    pStack, i
         CALL    INTERP
         CALL    HALT
;
         .SECTION "memvec", "rw"
;
;******* FORTH Globals
         .BLOCK  2           ;Padding
PSP:     .WORD   pStack      ;Current parameter stack pointer
RSP:     .WORD   rStack      ;Current return stack pointer
STATE:   .WORD   0           ;0=interpret, !0=compile
LATEST:  .WORD   _INTERP      ;Pointer to the most recently defined word
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
