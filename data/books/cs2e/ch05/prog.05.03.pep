;Program 5.3
LOADA   h#0011,d   ;A := first number
ADDA    h#0013,d   ;Add the two numbers
ORA     h#0015,d   ;Convert sum to character
STBYTA  h#0010,d   ;Store the character
CHARO   h#0010,d   ;Output the character
STOP  
.BLOCK  d#1        ;Character to output
.WORD   d#5        ;Decimal 5
.WORD   d#3        ;Decimal 3
.WORD   h#0030     ;Mask for ASCII char
.END   
