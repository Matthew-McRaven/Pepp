( Torture program for nested if-then-else
Should echo 0..3, but halt on any other number)
: T DUP 3 -
IF
  3 .
ELSE
  DUP 2 -
  IF
    2 .
  ELSE
    DUP 1 -
    IF
      1 .
    ELSE
      DUP 0 -
      IF
        0 .
      ELSE
        HALT
      THEN
    THEN
  THEN
THEN
;
4 T HALT
