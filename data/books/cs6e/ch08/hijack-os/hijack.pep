LDWX 0,s   ; X <- retAddr
; mainErr is not an exported symbol from OS.
; We can recover the value by accessing the argument of the BRNE
; Not using a fixed address beause the OS memory layout may change
LDWA 4,x   ; X <- &mainErr
STWA 0,s   ; retAddr <- &mainErr
RET        ; Return into mainErr
