         .SECTION ".STACK"
bp:      .WORD bp

         .SECTION "memvec"
         .INPUT  diskIn
diskIn:  .BLOCK  1
         .INPUT  charIn
         .EXPORT charIn
charIn:  .BLOCK  1
         .OUTPUT charOut
         .EXPORT charOut
charOut: .BLOCK  1
         .OUTPUT pwrOff
         .EXPORT pwrOff
pwrOff:  .BLOCK  1
.END