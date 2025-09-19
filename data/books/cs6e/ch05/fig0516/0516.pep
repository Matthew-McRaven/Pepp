         LDWA    DECI,i      ;Input width
         SCALL   width,d
         LDWA    DECI,i      ;Input height
         SCALL   height,d
         LDWA    width,d     ;Compute perimeter of rectangle
         ADDA    height,d
         ASLA
         STWA    perim,d
         LDWA    STRO,i      ;Output "width: "
         SCALL   msg1,d
         LDWA    DECO,i      ;Output width
         SCALL   width,d
         LDBA    '\n',i      ;Output newline character
         STBA    charOut,d
         LDWA    STRO,i      ;Output "height: "
         SCALL   msg2,d
         LDWA    DECO,i      ;Output height
         SCALL   height,d
         LDBA    '\n',i      ;Output newline character
         STBA    charOut,d
         LDWA    STRO,i      ;Output "perimeter: "
         SCALL   msg3,d
         LDWA    DECO,i      ;Output perimeter
         SCALL   perim,d
         RET
width:   .BLOCK  2
height:  .BLOCK  2
perim:   .BLOCK  2
msg1:    .ASCII  "width: \0"
msg2:    .ASCII  "height: \0"
msg3:    .ASCII  "perimeter: \0"
