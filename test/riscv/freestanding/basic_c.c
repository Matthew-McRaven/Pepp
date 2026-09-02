__asm__(".global _start\n"
        ".section .text\n"
        "_start:\n"
        "	j _start\n"
        "	nop\n");
