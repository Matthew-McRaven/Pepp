# Assembly Language, OS4

Reference: _Computer Systems_, Chapter 8, Section 8.2.

A trap instruction executes as if it were a single machine language instruction wired into the instruction set of the CPU.
However, it really executes a _sequence_ of machine language instructions that are initiated by the hardware trap mechanism.
The Pep/9 operating system provides two unary trap instructions, `NOP0` and `NOP1`, and five nonunary trap instructions, `NOP`, `DECI`, `DECO`, `HEXO`, and `STRO`, so you can reprogram them to implement instructions of your own choosing.
