# Data-movement instructions
The

|Operation                       |Mnemonic |Addressing Modes|Notes|
|--------------------------------|---------|----------------|-------------------------------------------------------------|
|Swap R with memory              | SWAPr   | AAA, no I      |Swap the value in R with the value in Mem[Oprnd]|
|Push word r onto stack          | PUSHWr  | Monadic        |For stack-oriented calculations, reduces # of bytes to bush from 6 to 1|
|POP word r from stack           | POPWr   | Monadic        |See PUSHWr|
|Move register R to PC           | MOVrPC  | Monadic        |PC <- R|
|Move PC to register R           | MOVPCr  | Monadic        |R <- PC; Most likely loaded into X to allow PC-relative loads|
|Sign extend byte in r           | SEBr    | Monadic        |Set bits r[15:8] to r[7]. Zero extension is redundant; it is the default behavior of LDBr|


Allow synthesis of more branch types, including indirect and pc-relative
```asm
LDWA ptrToInstr,sf
MOVAPC
```
PC-relative loads
```asm
MOVPCX
LDWr   offset, x
```

# Arithmetic instructions
MULLr  --- Multiply (lower word of product). r<- (r times operand)[15:0]
MULHr  --- Multiply (higher word of product, signed). r <- (r times operand)[31:16]
MULHUr --- Multiply (higher word of product, unsigned). r <- (r times operand)[31:16]
DIVr   --- Signed division. r <- r / op
DIVUr  --- Unsigned divsion. r <- r / op
REMr   --- Remainder from signed division
REMUr  --- Remainder from unsigned division

As an optimization, MUL / DIV should always compute the 32-bit result.
That result should be cached.
If the inputs match, pick the correct 16-bit result + flags.

If we drop the remainder instructions, we can compute the remainder with this sequence:
```asm
DIVA divisor,s ; A holds quotient
STWA quotient,s
MULL divisor,s
NEGA
ADDA dividend,s
STWA remainder,s
```
