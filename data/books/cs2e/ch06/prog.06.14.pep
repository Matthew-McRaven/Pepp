;Program 6.14
         BR      Main
width:   .BLOCK  d#2
height:  .BLOCK  d#2
perim:   .BLOCK  d#2
;
;------- void CalcRect (int& per, int wid, int ht)
per:     .EQUATE d#6        ;formal parameter
wid:     .EQUATE d#4        ;formal parameter
ht:      .EQUATE d#2        ;formal parameter
CalcRect:LOADA   wid,s      ;2 * (wid + ht)
         ADDA    ht,s
         ASLA
         LOADB   per,s      ;assign to per
         LOADX   d#0,i
         STOREA  ,x
         RTS                ;pop retAddr
;
;------- main()
Main:    DECI    width,d    ;cin >> width >> height
         DECI    height,d
         LOADA   perim,i    ;call by reference
         STOREA  d#-2,s
         LOADA   width,d    ;call by value
         STOREA  d#-4,s
         LOADA   height,d   ;call by value
         STOREA  d#-6,s
         ADDSP   d#-6,i     ;push parameters
         JSR     CalcRect   ;push retAddr
         ADDSP   d#6,i      ;pop parameters
         DECO    perim,d    ;cout << perim
         STOP
         .END

