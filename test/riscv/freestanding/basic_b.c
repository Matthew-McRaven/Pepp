__asm__(".global _start\n"
        ".section .text\n"
        "_start:\n"
        "	li a0, 666\n"
        "	li a7, 1\n"
        "	ecall\n"
        "	nop\n");
