# Writing Trap Handlers

Reference: _Computer Systems_, Chapter 8, Section 8.2.

A trap instruction executes as if it were a single machine language instruction wired into the instruction set of the CPU.
However, it really executes a _sequence_ of machine language instructions that are initiated by the hardware trap mechanism.
The Pep/9 operating system provides two unary trap instructions, `NOP0` and `NOP1`, and five nonunary trap instructions, `NOP`, `DECI`, `DECO`, `HEXO`, and `STRO`, so you can reprogram them to implement instructions of your own choosing.

Topics: [Trap handlers](#Trap), [The .BURN pseudo-op](#BURN).

### Trap handlers

To modify the operating system for the problems in Chapter 8 of the text is a six-step process.

#### Step 1.

Decide on your mnemonic for your new instruction. It will replace one of `NOP0`, `NOP1`, `NOP`, `DECI`, `DECO`, `HEXO`, or `STRO`.
Select the menu option System→Redefine Mnemonics to change the mnemonics of one of the instructions.

![redefinemnemonicsmenu](qrc:/help-asm/images/redefinemnemonicsmenu.png)

The dialog box requires you to enter a mnemonic and its allowed addressing modes if it is nonunary.
For example, change the mnemonic for the unary instruction `NOP0` to `ECHO`.

![redefinemnemonics](qrc:/help-asm/images/redefinemnemonics.png)

#### Step 2.

In this Help system in the pane on the left, select Pep/9 Operating System, and then click the Copy to Source button.
The default operating system will be copied to the Source Code pane.

![pep9oshelpsystem](qrc:/help-asm/images/pep9oshelpsystem.png)

#### Step 3.

Modify the trap handler part of the operating system to implement your new instruction.
As an example, here is the original NOP0 trap handler.

![nop0](qrc:/help-asm/images/nop0.png)

And here is how you would modify it to implement the new `ECHO` instruction in place of `NOP0`.

![echo](qrc:/help-asm/images/echo.png)

CAUTION: You cannot use any trap instructions in your trap handler.

#### Step 4.

Select System→Assemble & Install New OS to assemble and install the reprogrammed operating system.

![assembleinstallnewos](qrc:/help-asm/images/assembleinstallnewos.png)

A message will inform you if your modified operating system assembled correctly.

![osinstalledmsg](qrc:/help-asm/images/osinstalledmsg.png)

You can save your modified operating system as a `.pep` file as you would any other Pep/9 assembly language program.

#### Step 5.

Write an assembly language program to test your new instruction with the new mnemonic.
The assembler should recognize the new mnemonic and generate the appropriate object code.
For example, your test program might be the following.

![echoprogram](qrc:/help-asm/images/echoprogram.png)

#### Step 6.

Run or Start Debugging your program written in step 5.
In this example, whatever character is placed in the Input pane should be echoed in the output pane.

![pecho](qrc:/help-asm/images/pecho.png)

[Scroll to topics](#Topics).

### The .BURN pseudo-op

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

[Scroll to topics](#Topics).
