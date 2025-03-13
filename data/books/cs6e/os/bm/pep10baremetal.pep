         .SECTION "stack", "rwz"
bmRAM:   .BLOCK  2
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
msg:     .ASCII "Cannot use system calls in bare metal mode\0"
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
