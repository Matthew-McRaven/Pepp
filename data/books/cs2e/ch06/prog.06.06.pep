;Program 6.6
         BR      Main
n1:      .BLOCK  d#2
n2:      .BLOCK  d#2
n3:      .BLOCK  d#2
;
Main:    DECI    n2,d
         DECI    n3,d
         LOADA   n2,d
         COMPA   n3,d
         BRLT    L1
         DECI    n1,d
         LOADA   n1,d
         COMPA   n3,d
         BRLT    L7
         BR      L6
         STOREA  n3,d
L1:      DECI    n1,d
         LOADA   n2,d
         COMPA   n1,d
         BRLT    L5
         DECO    n1,d
         DECO    n2,d
L2:      DECO    n3,d
         STOP
L3:      DECO    n2,d
         DECO    n3,d
         BR      L9
L4:      DECO    n1,d
         DECO    n2,d
         STOP
         STOREA  n1,d
L5:      LOADA   n3,d
         COMPA   n1,d
         BRLT    L3
         DECO    n2,d
         DECO    n1,d
         BR      L2
L6:      DECO    n3,d
         LOADA   n1,d
         COMPA   n2,d
         BRLT    L4
         BR      L8
L7:      DECO    n1,d
         DECO    n3,d
         DECO    n2,d
         STOP
L8:      DECO    n2,d
L9:      DECO    n1,d
         STOP
         .END

