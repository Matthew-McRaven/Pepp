;Stan Warford
;May 1, 2016
;A program to output "Hi"
;
         LDBA    0x000F,d    ;Load byte accumulator 'H'
         STBA    0xFAAC,d    ;Store byte accumulator output device
         LDBA    0x0010,d    ;Load byte accumulator 'i'
         STBA    0xFAAC,d    ;Store byte accumulator output device
         STBA    0xFAAE,d    ;Store byte to power off port
         .ASCII  "Hi"        ;ASCII "Hi" characters
         .END
