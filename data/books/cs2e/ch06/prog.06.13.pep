;Program 6.13
         BR      Main
i:       .BLOCK  d#2        ;reserve 1 word
v:       .BLOCK  d#8        ;reserve 4 words


lineFeed:.EQUATE h#000A
;
Main:    LOADB   v,i        ;B := address of v
         LOADX   d#0,i      ;initialize i = 0
         STOREX  i,d
For1:    COMPX   d#4,i      ;test for i < 4
         BRGE    EndFor1
         ASLX               ;X := X*2 for words
         DECI    ,x         ;cin >> v[i]
         LOADX   i,d        ;increment i++
         ADDX    d#1,i
         STOREX  i,d
         BR      For1
EndFor1: LOADX   d#3,i      ;initialize i = 3
         STOREX  i,d
For2:    COMPX   d#0,i      ;test for i >= 0
         BRLT    EndFor2
         DECO    i,d        ;cout << i  << "  "
         CHARO   c#/ /,i
         CHARO   c#/ /,i
         ASLX               ;X := X*2 for words
         DECO    ,x         ;cout << v[i] << endl
         CHARO   lineFeed,i
         LOADX   i,d        ;decrement i--
         SUBX    d#1,i
         STOREX  i,d
         BR      For2
EndFor2: STOP
         .END

