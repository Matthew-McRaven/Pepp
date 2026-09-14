;Program 6.8
LOADA   c#/r/,i    ;put 'r' on stack
STBYTA  d#-1,s
LOADA   c#/e/,i    ;put 'e' on stack
STBYTA  d#-2,s
LOADA   c#/a/,i    ;put 'a' on stack
STBYTA  d#-3,s
LOADA   c#/l/,i    ;put 'l' on stack
STBYTA  d#-4,s
ADDSP   d#-4,i     ;push 'real'
CHARO   d#2,s      ;output 'e'
CHARO   d#1,s      ;output 'a'
CHARO   d#3,s      ;output 'r'
ADDSP   d#4,i      ;pop 'real'
STOP
.END

