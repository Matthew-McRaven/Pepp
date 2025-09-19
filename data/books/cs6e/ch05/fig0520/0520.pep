         @DECI   width,d     ;Input width
         @DECI   height,d    ;Input height
         LDWA    width,d     ;Compute perimeter of rectangle
         ADDA    height,d
         ASLA
         STWA    perim,d
         @STRO   msg1,d      ;Output "width: "
         @DECO   width,d     ;Output width
         @CHARO  '\n',i      ;Output newline character
         @STRO   msg2,d      ;Output "height: "
         @DECO   height,d    ;Output height
         @CHARO  '\n',i      ;Output newline character
         @STRO   msg3,d      ;Output "perimeter: "
         @DECO   perim,d     ;Output perimeter
         RET
width:   .BLOCK  2
height:  .BLOCK  2
perim:   .BLOCK  2
msg1:    .ASCII  "width: \0"
msg2:    .ASCII  "height: \0"
msg3:    .ASCII  "perimeter: \0"
