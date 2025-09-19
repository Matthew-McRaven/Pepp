         LDWA    DECO,i
         SCALL   num,d
         LDBA    ' ',i
         STBA    charOut,d
         LDWA    HEXO,i
         SCALL   num,d
         RET
num:     .WORD   -3
