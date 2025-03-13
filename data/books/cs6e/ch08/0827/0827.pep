;File: prob0827.pep
;Computer Systems, Fifth edition
;Problem 8.27 test program
;
         BR      main        ;branch around data
num:     .BLOCK  2           ;global variable
count:   .BLOCK  2           ;global variable
flags:   .BLOCK  1           ;flags from shift
;
main:    DECI    num,d       ;input decimal value
         DECI    count,d     ;input shift count
         STRO    msg1,d      ;output original value
         DECO    num,d       
         LDBA    '\n',i      
         STBA    charOut,d   
         LDWA    num,d       ;test ASLMANY instruction
         ASLMANY count,d     
         STWA    num,d       
         MOVFLGA             ;store flags
         STBA    flags,d     
         STRO    msg2,d      ;output shifted value
         DECO    num,d       
         LDBA    '\n',i      
         STBA    charOut,d   
testN:   LDBA    flags,d     ;test N
         ANDA    0x0008,i    
         BREQ    outN0       
         STRO    msgN1,d     ;output "N = 1"
         BR      testZ       
outN0:   STRO    msgN0,d     ;output "N = 0"
testZ:   LDBA    flags,d     ;test Z
         ANDA    0x0004,i    
         BREQ    outZ0       
         STRO    msgZ1,d     ;output "Z = 1"
         BR      testV       
outZ0:   STRO    msgZ0,d     ;output "Z = 0"
testV:   LDBA    flags,d     ;test V
         ANDA    0x0002,i    
         BREQ    outV0       
         STRO    msgV1,d     ;output "V = 1"
         BR      testC       
outV0:   STRO    msgV0,d     ;output "V = 0"
testC:   LDBA    flags,d     ;test C
         ANDA    0x0001,i    
         BREQ    outC0       
         STRO    msgC1,d     ;output "C = 1"
         BR      halt        
outC0:   STRO    msgC0,d     ;output "C = 0"
halt:    STOP                
msg1:    .ASCII  "Original value = \0"
msg2:    .ASCII  "Shifted value = \0"
msgN0:   .ASCII  "N = 0\n\0"
msgN1:   .ASCII  "N = 1\n\0"
msgZ0:   .ASCII  "Z = 0\n\0"
msgZ1:   .ASCII  "Z = 1\n\0"
msgV0:   .ASCII  "V = 0\n\0"
msgV1:   .ASCII  "V = 1\n\0"
msgC0:   .ASCII  "C = 0\n\0"
msgC1:   .ASCII  "C = 1\n\0"
         .END                  
