# CPU Display
The Pep/9 debugging system lets you step through the execution of a program one statement at a time.
At each point in time, the Program Trace pane highlights the assembly language statement that will be executed next, and the CPU displays the contents of its registers just after the previous statement has executed.

The screenshot of the Program Trace below highlights the `ASRA` instruction at 0012.
This statement has _not_ yet executed. The screenshot of the CPU display below shows the state of the processor just _after_ the `ADDA` instruction at 000F has executed.

![cpudisplay1](qrc:/help-asm/images/cpudisplay1.png)

![cpudisplay2](qrc:/help-asm/images/cpudisplay2.png)

The CPU pane shows the values of the NZVC bits and of each register in the CPU.
It displays the content of the accumulator, index register, stack pointer, and program counter first in hexadecimal and then in decimal.
In the above screenshot, the accumulator has a decimal value of 152, the result of adding 68 for `exam1` and 84 for `exam2`.

The CPU display shows the instruction specifier first in binary, and then as a mnemonic.
If the instruction is nonunary, it decodes the addressing mode field and shows the corresponding letter for the addressing mode after the mnemonic, and displays the operand specifier.
In the above screenshot, the mnemonic is `ADDA`, and the addressing mode letter is `s` for stack-relative addressing.
The operand specifier is 2.

The operand is not part of the CPU, except for the case of immediate addressing when the operand _is_ the operand specifier.
However, for the convenience of the programmer, the CPU pane computes the value of the operand from the addressing mode and displays it as well.
The Operand field in the CPU pane is enclosed in parentheses to emphasize the fact that it is _not_ part of the CPU.
In the above screenshot, the operand is 84, the decimal value that was just added to the accumulator.

The values in the CPU pane are the values _after_ the instruction in the instruction specifier has executed.
For example, the values in the above screenshots are the values after the `ADDA` at 000F executes.
In the von Neumann cycle, the program counter increments before the instruction executes.
So, the value of the program counter is the address of the _next_ instruction to execute.
In the above screenshot, the program counter is 0012, which is the address of the _next_ instruction to execute, while the instruction that just executed is the instruction at address 000F.
