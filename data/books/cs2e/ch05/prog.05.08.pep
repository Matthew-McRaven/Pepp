;Program 5.8
         BR      Main       ;Branch around data
first:   .WORD   d#256
second:  .WORD   h#8000
lineFeed:.EQUATE h#000A     ;ASCII line feed
;
Main:    DECO    first,d    ;Interpret first as dec
         CHARO   lineFeed,i
         HEXO    first,d    ;Interpret first as hex
         CHARO   lineFeed,i
         DECO    second,d   ;Interpret second as dec
         CHARO   lineFeed,i
         HEXO    second,d   ;Interpret second as hex
         STOP  
         .END   
