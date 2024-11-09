         .SECTION "stack", "rwz"
bmRAM:   .BLOCK  2
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
         .SECTION "text", "wx"
;
;******* Trap Handler
;While bare metal mode is not supposed to have a trap handler,
;failure to provide one could cause user-programs to enter an infinite loop.
;This trap handler will print out an error message before terminating execution.
trp:     LDWX    0,i         ;X <- 0
prntMore:LDBA    msg,x       ;Test next char
         BREQ    exitPrnt    ;If null then exit
         STBA    charOut,d   ;else print
         ADDX    1,i         ;X <- X + 1 for next character
         BR      prntMore
exitPrnt:LDWA    0xDEAD, i
         STBA    pwrOff, d
hang:    BR      hang
msg:     .ASCII "Cannot use system calls in bare metal mode\x00"
;
trpHnd:   .WORD  trp
initPC:  .WORD   0x0000
;
         .SECTION "memvec", "rw"
         .ORG    0xFFFB
initSP:  .WORD   bmRAM
         .INPUT  charIn
         .EXPORT charIn
charIn:  .BLOCK  1
         .OUTPUT charOut
         .EXPORT charOut
charOut: .BLOCK  1
         .OUTPUT pwrOff
         .EXPORT pwrOff
pwrOff:  .BLOCK  1
