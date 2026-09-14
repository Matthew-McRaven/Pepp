;Program 5.5
BR      h#0005     ;Branch around data
.BLOCK  d#2        ;Storage for one integer
;
DECI    h#0003,d   ;Get the number
DECO    h#0003,d   ;and output it
CHARO   c#/ /,i    ;Output ' + 1 = '
CHARO   c#/+/,i
CHARO   c#/ /,i
CHARO   c#/1/,i
CHARO   c#/ /,i
CHARO   c#/=/,i
CHARO   c#/ /,i
LOADA   h#0003,d   ;A := the number
ADDA    d#1,i      ;Add one to it
STOREA  h#0003,d   ;Store the sum
DECO    h#0003,d   ;Output the sum
STOP  
.END   
