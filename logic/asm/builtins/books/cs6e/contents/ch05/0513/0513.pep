;File: fig0513.pep
;Computer Systems, Fifth edition
;Figure 5.13
;
         BR      0x0009      ;Branch around data
         .WORD   0xFFFE      ;First
         .BYTE   0x00        ;Second
         .BYTE   'U'         ;Third
         .WORD   1136        ;Fourth
;
         LDWT    DECO,i      ;Interpret First as dec
         SCALL   0x0003,d   
         LDBA    '\n',i      
         STBA    0xFAAC,d    
         LDWT    DECO,i      ;Interpret Second and Third as dec
         SCALL   0x0005,d   
         LDBA    '\n',i      
         STBA    0xFAAC,d    
         LDWT    HEXO,i      ;Interpret Second and Third as hex
         SCALL   0x0005,d    
         LDBA    '\n',i      
         STBA    0xFAAC,d    
         LDBA    0x0006,d    ;Interpret Third as char
         STBA    0xFAAC,d    
         LDBA    0x0008,d    ;Interpret Fourth as char
         STBA    0xFAAC,d    
         RET                
         .END                  
