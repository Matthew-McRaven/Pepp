;Stan Warford
;May 1, 2016
;A program to output "Hi"
;
         LDBA    0x000F,d    ;Load byte accumulator 'H'
         STBA    charOut,d    ;Store byte accumulator output port
         LDBA    0x0010,d    ;Load byte accumulator 'i'
         STBA    0xFFFE,d    ;Store byte accumulator output port
         STBA    0xFFFF,d    ;Store byte accumulator power off port
         .ASCII  "Hi"        ;ASCII "Hi" characters
