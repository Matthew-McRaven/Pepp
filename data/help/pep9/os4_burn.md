# The .BURN pseudo-op

When you include `.BURN` in a program, the assembler assumes that the program will be burned into read-only memory (ROM).
It generates code for those instrctions that follow the burn directive, but not for those that precede it.
The assembler also assumes that the ROM will be installed at the bottom of memory, leaving the top of memory for the application programs.
It therefore calculates that addresses for the symbol table such that the last byte generated will have the address specified by the burn directive.

Pep/9 is a 16-bit computer, and can therfore access 216 = 64 KiB = 65,536 bytes of main memory.
The Pep/9 operating system contains `.BURN 0xFFFF` because `0xFFFF` is the address of the 65,536th byte.
The following figure shows the memory dump at the bottom of memory.
Figure 8.1 of the textbook shows that machine vector FC52, which is the address of the first instruction of the trap handler, is stored at FFFE.

![relocatedos3](qrc:/help-asm/images/relocatedos3.png)

It is possible to install a smaller amount of memory in the Pep/9 system.
Simply change the burn directive in the operating system to a smaller value.
For example, you can change the burn directive to `.BURN 0x7FFF`, assemble and install the new OS, and the system will install only 32 KiB of memory instead of 64 KiB.

![modifieddotburn](qrc:/help-asm/images/modifieddotburn.png)

The last machine vector is now at address 7FFF instead of FFFF.
Furthermore, the machine vector is now 7C52 instead of FC52, because 7C52 is now the address of the first instruction of the trap handler.
The memory dump displays zz for uninstalled memory.

![relocatedos2](qrc:/help-asm/images/relocatedos2.png)

The trap handlers all run correctly, which you can trace in the Memory Dump pane.
Following is the memory dump after the execution of a `DECI` trap instruction.
A trap stores the ten bytes of the process control block on the system stack, indicated by the ten red bytes in the memory dump.
Note that the first instruction in the trap handler to execute is DB 0009 and is stored at 7C52. With 64 KiB of memory installed, it would be at address FC52.

![relocatedos](qrc:/help-asm/images/relocatedos.png)
