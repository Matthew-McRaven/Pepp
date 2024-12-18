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
;
;******* FORTH Globals
         .BLOCK  2           ;Padding
PSP:     .WORD   pStack      ;Current parameter stack pointer
RSP:     .WORD   rStack      ;Current return stack pointer
STATE:   .WORD   0           ;0=interpret, !0=compile
LATEST:  .WORD   _STORE      ;Pointer to the most recently defined word
HERE:    .WORD   0x0000      ;Pointer to the next free memory location
         .SECTION "text", "rx"
;******* FORTH Constants

;******* FORTH words
@DC      HALT, 0x0000, 0x05, 0x09
HALT:    LDWA    0xDEAD, i
         STBA    pwrOff, d
hang:    BR      hang

@DC      DROP, _HALT, 0x05, 0x04
         ADDX    2,i         ;Drop top of parameter stack stack
         RET

;Also tried implementing with XOR-swap, but that used 3 more bytes.
@DC      SWAP, _DROP, 0x05, 0x13
         LDWA    0,x         ;Load TOS+0, store to TOS-1
         STWA    -2,x
         LDWA    2,x         ;Load TOS+1, store to TOS+0
         STWA    0,x
         LDWA    -2,x        ;Load TOS-1, store to TOS+1
         STWA    2,x
         RET

@DC      DUP, _SWAP, 0x04, 0x0A
DUP:     LDWA    0,x         ;Load TOS+0, store to TOS-1
         STWA    -2,x
         SUBX    2,i         ;Decrement PSP
         RET

@DC      OVER, _DUP, 0x05, 0x0A
         LDWA    2,x         ;Load TOS+1, store to TOS-1
         STWA    -2,x
         SUBX    2,i         ;Decrement PSP
         RET

@DC      ROT, _OVER, 0x04, 0x1C
         LDWA    0,x         ;Load TOS+0, store to TOS-1
         STWA    -2,x
         LDWA    2,x         ;Load TOS+1, store to TOS+0
         STWA    0,x
         LDWA    4,x         ;Load TOS+2, store to TOS+1
         STWA    2,x
         LDWA    -2,x        ;Load TOS-1, store to TOS+2
         STWA    4,x
         RET
;Ignoring -ROT for now since I don't (yet) need it.
@DCSTR   "2DROP\x00", DROP2, _ROT, 0x06, 0x05
         ADDX    4,i         ;Drop top two elements of parameter stack
         RET

@DCSTR   "2DUP\x00", DUP2, _DROP2, 0x05, 0x0A
         LDWA    0,x         ;Load TOS+0, store to TOS-2
         STWA    -4,x
         LDWA    2,x         ;Load TOS+1, store to TOS-1
         STWA    -2,x
         SUBX    4,i         ;Decrement PSP
         RET

@DCSTR   "?DUP\x00", MDUP, _DUP2, 0x05, 0x07
         LDWA    0,x         ;Load TOS+0
         BRNE    DUP         ;If non-0, DUP
         RET
@DCSTR   "1+\x00", INCR, _MDUP, 0x03, 0x0A
         LDWA    0,x         ;Increment TOS by 1
         ADDA    1,i
         STWA    0,x
         RET
@DCSTR   "1-\x00", DECR, _INCR, 0x03, 0x0A
         LDWA    0,x         ;Decrement TOS by 1
         SUBA    1,i
         STWA    0,x
         RET
@DCSTR   "2+\x00", INCR2, _DECR, 0x03, 0x0A
         LDWA    0,x         ;Increment TOS by 2
         ADDA    2,i
         STWA    0,x
         RET
@DCSTR   "2-\x00", DECR2, _INCR2, 0x03, 0x0A
         LDWA    0,x         ;Decrement TOS by 2
         SUBA    2,i
         STWA    0,x
         RET
@DCSTR   "+\x00", ADD, _DECR2, 0x02, 0x0D
         LDWA    -2,x         ;Add TOS to TOS-1
         ADDA    0,x
         STWA    -2,x
         ADDX    2,i
         RET
@DCSTR   "-\x00", SUB, _ADD, 0x02, 0x0D
         LDWA    -2,x         ;Sub TOS from TOS-1
         SUBA    0,x
         STWA    -2,x
         ADDX    2,i
         RET
@DC      AND, _SUB, 0x04, 0x0D
         LDWA    -2,x         ;Bitwise AND TOS and TOS-1
         ANDA     0,x
         STWA    -2,x
         ADDX    2,i
         RET
@DC      OR, _AND, 0x03, 0x0D
         LDWA    -2,x         ;Bitwise OR TOS and TOS-1
         ORA    0,x
         STWA    -2,x
         ADDX    2,i
         RET
@DC      XOR, _OR, 0x04, 0x0D
         LDWA    -2,x         ;Bitwise XOR TOS and TOS-1
         XORA     0,x
         STWA    -2,x
         ADDX    2,i
         RET
@DCSTR   "INVERT\x00", INV, _XOR, 0x07, 0x08
         LDWA    0,x          ;Bitwise NOT TOS
         NOTA
         STWA    0,x
         RET
@DCSTR   "@\x00", LOAD, _INV, 0x02, 0x10
         STWX    PSP,d        ;Store PSP to global data
         LDWX    0,x          ;Get the word pointed to by PSP
         LDWA    0,x          ;Dereference that word
         LDWX    PSP,d        ;Restore PSP
         STWA    0,x          ;Store the word to TOS
         RET
@DCSTR   "!\x00", STORE, _LOAD, 0x02, 0x10
         LDWA    2,x          ;Load TOS-1 into A
         STWX    PSP,d        ;Store PSP to global data
         LDWX    0,x          ;Get the word pointed to by PSP
         STWA    0,x          ;Store TOS-1
         LDWX    PSP,d        ;Restore PSP
         RET
@DVAR    STATE, _STORE, 0x06
@DVAR    LATEST, _STATE, 0x07
@DVAR    HERE, _LATEST, 0x05
;
;******* FORTH interpreter
cldstrt: LDWX    pStack, i
         CALL    HALT
;
;******* Trap Handler
;While bare metal mode is not supposed to have a trap handler,
;failure to provide one could cause user-programs to enter an infinite loop.
;This trap handler will print out an error message before terminating execution.
trp:     LDWX    0,i         ;X <- 0
prntMore:LDBA    msg,x       ;Test next char
         BREQ    HALT        ;If null then exit
         STBA    charOut,d   ;else print
         ADDX    1,i         ;X <- X + 1 for next character
         BR      HALT
msg:     .ASCII "Cannot use system calls in bare metal mode\x00"
;
trpHnd:  .WORD   trp
initPC:  .WORD   cldstrt
;
         .SECTION "memvec", "rw"
         .ORG    0xFFFB
initSP:  .WORD   rStack
         .INPUT  charIn
charIn:  .BLOCK  1
         .OUTPUT charOut
charOut: .BLOCK  1
         .OUTPUT pwrOff
pwrOff:  .BLOCK  1
