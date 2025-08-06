dstAddr: .EQUATE 0           ; Destination of copy #2h
offset:  .EQUATE 2           ; Difference between baseAddr and dstAddr #2h
baseAddr:.EQUATE 4           ; Address from which copy starts #2h
limit:   .EQUATE 6           ; Number of bytes to copy #2h
         CPWA    0,i         ; Dummy instruction so that baseAddr is not 0x0000
         CALL    calcBase    ; Use return address to compute address of this intsuction
         SUBSP   8,i         ; #limit#baseAddr#offset#dstAddr
         STWA    baseAddr,s
         @DECI   dstAddr,s   ; Determine target address using DECI
         LDWA    dstAddr,s   ; offset <- dstAddr - baseAddr
         BREQ    halt        ; If dstAddr == 0, stop replicating
         SUBA    baseAddr,s
         STWA    offset,s
         CALL    calcLim     ;limit <- &last_instruction - baseAddr
         SUBA    baseAddr,s
         STWA    limit,s
         LDWX    0,i         ; Iterate from [0, limit)
loop:    CPWX    limit,s
         BRGE    stop
         LDBA    baseAddr,sfx;Treat this program as an array whose address is on the stack
         STBA    dstAddr,sfx ;Treat destination as an array whose address is on the stack
         ADDX    1,i         ;X++ for opcode copy
         CPWA    0x0023,i    ;If A is monadic
         BRLE    loop        ;  copy the next instruction
; [0x28,0x37] are branch opcodes
; Program is specifically designed so only BR instructions have absolute (direct) addressing
; Arguments of these opcodes need relocation
         CPWA    0x0037,i
         BRLE    rela
; All other opcodes' arguments can just be copied
         LDWA    baseAddr,sfx
         BR      wb
rela:    LDWA    baseAddr,sfx
; Perform relocation of branch instruction arguments
         ADDA    offset,s
wb:      STWA    dstAddr,sfx
         ADDX    2,i
         BR      loop
stop:    LDWA    dstAddr,s
         ADDSP   8,i         ; #dstAddr#offset#baseAddr#limit
         STWA    0,s
         RET
halt:    LDWA    0xDEAD,i
         STBA    pwrOff,d
self:    BR      self        ; If pwrOff is broken, enter infinite loop.
; Use retAddr - 3 to compute baseAddress
calcBase:LDWA    0,s
         SUBA    3,i
         RET
; *[retAddr-2] contains address of this instruction.
; Add 10 to get address of return, which is last instruction in this program
calcLim: LDWX    -2,i
         LDWA    0,sfx
         ADDA    10,i
         RET
