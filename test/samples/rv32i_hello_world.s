# A simple hello-world program to exercise our new risc-v simulator.
# Assume the MMIO base address is loaded into x31, and charOut is at +4 from it.
addi t1, x0, 104    # 'h'
sb   t1, 4(x31)
addi t1, x0, 101    # 'e'
sb   t1, 4(x31)
addi t1, x0, 108    # 'l' l'
sb   t1, 4(x31)
sb   t1, 4(x31)
addi t1, x0, 111    # 'o'
sb   t1, 4(x31)
sb   x0, 8(x31)     # Shutdown