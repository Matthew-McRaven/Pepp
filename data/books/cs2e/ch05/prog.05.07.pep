;Program 5.7
         BR      Main       ;Branch around data
num:     .BLOCK  d#2        ;Storage for one integer
;
Main:    DECI    num,d      ;Get the number
         DECO    num,d      ;and output it
         CHARO   c#/ /,i    ;Output ' + 1 = '
         CHARO   c#/+/,i
         CHARO   c#/ /,i
         CHARO   c#/1/,i
         CHARO   c#/ /,i
         CHARO   c#/=/,i
         CHARO   c#/ /,i
         LOADA   num,d      ;A := the number
         ADDA    d#1,i      ;Add one to it
         STOREA  num,d      ;Store the sum
         DECO    num,d      ;Output the sum
         STOP  
         .END   
