# The entry point, followed by data and zeroed memory. Nothing here needs a relocation, but b.s refers to _start.
.globl _start
.text
_start: nop
li a0, 5
.data
.word 0x11223344
.bss
.space 8
