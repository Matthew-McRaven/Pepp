# More of each section, and .mine, which the script does not place. Its second word needs an R_RISCV_32.
.text
nop
.data
.byte 7
.section .mine,"aw"
.word 0x55667788
.word _start + 4
