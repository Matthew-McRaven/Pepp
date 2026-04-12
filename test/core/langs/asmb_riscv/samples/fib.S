# Compute the first 4 fib numbers and store them in increasing registers.
_start:addi x1, x0, 1
addi x2, x0, 1
add  x3, x1, x2
# Store the 4th number to0*0x3 and reload it into x4.
sw   x3,0(x0)
lw   x4,0(x0)
# Add n-2 and n-1 (currently in x4) to get n.
add  x4, x2, x4
# Test out U-type instructions
lui  x0, %hi(0xfeedbeef)
