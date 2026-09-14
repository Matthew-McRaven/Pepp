;Program 5.6
BR      h#0007     ;Branch around data
.WORD   d#256      ;First
.WORD   h#8000     ;Second
;
DECO    h#0003,d   ;Interpret first as dec
CHARO   h#000A,i   ;Output linefeed
HEXO    h#0003,d   ;Interpret first as hex
CHARO   h#000A,i   ;Output linefeed
;
DECO    h#0005,d   ;Interpret second as dec
CHARO   h#000A,i   ;Output linefeed
HEXO    h#0005,d   ;Interpret second as hex
STOP  
.END   
