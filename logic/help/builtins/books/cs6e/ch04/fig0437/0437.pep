LDBA oc,d
STBA ds,d
LDWA fst,d
ds:ADDA snd,d
ORA mask,d
STBA 0xFFFE,d
STBA 0xFFFF,d
fst:.WORD 0x05
snd:.WORD 0x03
mask:.WORD 0x30
oc:suba 00,d
