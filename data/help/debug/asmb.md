# Tracing assembler code

If you are debugging an assembly language program that you wrote, enter the program in the Source Code pane.
Or, if you want to trace a textbook example, select it from the column on the left and click the Copy to Source button.

![copytosource](qrc:/help-asm/images/copytosource.png)

Select Debug→Start Debugging from the menu, or click on the icon with the bug in the tool bar.
The Start Debugging action does three things — clears memory to all zeros, assembles and loads the source program, and puts you in the debugging mode.

![startdebuggingsource](qrc:/help-asm/images/startdebuggingsource.png) ![startdebuggingicon](qrc:/help-asm/images/startdebuggingicon.png)

The following sequence of figures shows the tracing steps when debugging the program of Figure 6.4 with values 68 84 in the input stream.

![fig0604input](qrc:/help-asm/images/fig0604input.png)

The Start Debugging action activates the Debugger tab, which displays the Program Trace.
The Program Trace shows the assembly language source code with the statement to be executed next highlighted in blue. With this program, the `BR main` instruction is about to execute.

![fig0604trace1](qrc:/help-asm/images/fig0604trace1.png)

## Single step

While you are in the debugging mode, you can select Debug→Single Step from the menu or click on the corresponding icon in the tool bar.

![singlestep](qrc:/help-asm/images/singlestep.png) ![singlestepicon](qrc:/help-asm/images/singlestepicon.png)

The debugger will perform one von Neumann cycle.
Namely, it will fetch the instruction, decode it, increment PC, and execute the instruction fetched.
In this example, the branch instruction uses immediate addressing to put 0003 into PC.
At each step of the trace you can also see the content of the CPU.
At this point, the CPU pane shows the instruction specifier for `BR` in binary and the operand specifier 0003 in hexadecimal. This is the instruction that was fetched and executed.
The value of PC is the address of the instruction that will be executed next, in this case the `SUBSP` instruction.
The program trace highlights the instruction that will be executed next, not the instruction that was just executed.

![fig0604trace2](qrc:/help-asm/images/fig0604trace2.png)

![fig0604trace2cpu](qrc:/help-asm/images/fig0604trace2cpu.png)

The following figures show the Program Trace and CPU after another single step, which executes the `SUBSP` instruction.
The CPU shows the `SUBSP` instruction in the instruction specifier and 6 in the operand specifier.
This is the instruction that was just executed. Also shown is the rendering of the run-time stack in the Memory Trace.
The program trace highlights the `DECI` instruction, which will be executed next.

![fig0604trace3](qrc:/help-asm/images/fig0604trace3.png)

![fig0604trace3cpu](qrc:/help-asm/images/fig0604trace3cpu.png) ![fig0604trace3stack](qrc:/help-asm/images/fig0604trace3stack.png)

Now the `DECI` instruction is about to execute.
But, `DECI` is a trap instruction, which generates a trap to the operating system.
Therefore, you have two trace options — (1) single step the trap instruction, or (2) step into the trap instruction.

## Single step a trap instruction

Option (1): If you select Single Step again, the debugger will execute all the code in the operating system for the `DECI` trap instruction and then show the state of the computer as if `DECI` were a single instruction.
The Program Trace below highlights the second `DECI` instruction, because that is the instruction that will be executed next.
The Memory Trace shows that the `DECI` instruction input the value 68 to `exam1` on the run-time stack.
The instruction specifier in the CPU shows that the last instruction executed is `RETTR`, which stands for "return from trap".
This is the instruction in the operating system that returns control to the application.

![fig0604trace4](qrc:/help-asm/images/fig0604trace4.png)

![fig0604trace4cpu](qrc:/help-asm/images/fig0604trace4cpu.png) ![fig0604trace4stack](qrc:/help-asm/images/fig0604trace4stack.png)

The Memory Dump pane also highlights the top byte of the stack in magenta.
The following screenshot show the stack starting at address FB89.
The `DECI` instruction stored 0044(hex) = 68(dec) at address FB8D on the run-time stack.

![memorydumpsp](qrc:/help-asm/images/memorydumpsp.png)

## Step into a trap instruction

Option (2): If you want to trace the execution of the statements in the operating system that implement the `DECI` instruction, you can choose the second trace option.
Select Debug→Step Into from the menu or click on the corresponding icon in the tool bar.

![stepinto](qrc:/help-asm/images/stepinto.png) ![stepintoicon](qrc:/help-asm/images/stepintoicon.png)

The Program Trace pane switches to the operating system.
When a trap instruction executes, control is transferred to the trap handler of the OS.
The first instruction of the trap handler to execute is the `LDBX` instruction at FC52.

![fig0604trace5](qrc:/help-asm/images/fig0604trace5.png) ![fig0604trace5cpu](qrc:/help-asm/images/fig0604trace5cpu.png)

## Step out

You can continue to single step through the operating system to trace the statements that implement `DECI`.
Eventually, you will execute `RETTR`, the return from trap instruction.
The Program Trace pane will switch back to the application and highlight the instruction after the one that caused the trap.
If you are part way through tracing the trap handler and want to immediately return to the application, select Debug→Step Out from the menu or click on the corresponding icon in the tool bar.

![stepout](qrc:/help-asm/images/stepout.png) ![stepouticon](qrc:/help-asm/images/stepouticon.png)

The debugger will automatically execute all the remaining statements of the code you are tracing up to, and including, the next `RETTR` or `RET` instruction.
You will then be ready to single step through the remaining statements of the application.

## Step over

If you are stepping through a program in the debugger and get to a `call` instruction, it is possible to bypass all the statements inside the function.
For example, susppose you trace the main program in Figure 6.25 of the textbook up to the `CALL` at 007D.
The `SUBSP` statement that just executed has pushed storage for `retVal` and parameters `n` and `k` onto the run-time stack.

![fig0625trace1](qrc:/help-asm/images/fig0625trace1.png) ![fig0625trace1stack](qrc:/help-asm/images/fig0625trace1stack.png)

If you do not want to trace all the code of the function but just want to know the state of the computation after the function returns, select Debug→Step Over from the menu or click the corresponding icon in the menu bar.

![stepover](qrc:/help-asm/images/stepover.png) ![stepovericon](qrc:/help-asm/images/stepovericon.png)

The debugger will execute all the statements of the function up to and including the `RET` statement that corresponds to the `CALL`.
In the following figure, the function has computed the value of `retVal`, which the main program will output.

![fig0625trace2](qrc:/help-asm/images/fig0625trace2.png) ![fig0625trace2stack](qrc:/help-asm/images/fig0625trace2stack.png)

The Step Over operation works on a `CALL` instruction the same way the Single Step operation works on a trap instruction.
In both cases, the debugger executes all the code of the trap or call as if it were a single instruction at a higher level of abstraction.

## Summary of step operations

Here is a summary of the four debugging step operations.

Step Over
*   Steps over a trap instruction.
*   Steps over a `CALL` instruction.

Step Into
*   Steps into a trap instruction.
*   Steps into a `CALL` instruction.

Single Step
*   Steps over a trap instruction.
*   Steps into a `CALL` instruction.

Step Out
*   Steps out of the most recent `call` or trap instruction.

Strictly speaking, the Single Step operation is not necessary, because you could always achieve the same effect by stepping over or stepping into the instruction.
It is provided as a convenience, because most of the time you will want to step over a trap instruction and step into a `CALL` instruction.
See the menu for the keyboard shortcut on your system for the Single Step operation.

# The Run Source option

The three steps — Assemble, Load, Execute — are combined in the single option called Run Source.
You can initiate a run by selecting Build→Run Source from the menu or by clicking on the Run Source icon in the tool bar.

![runsource](qrc:/help-asm/images/runsource.png) ![runicon](qrc:/help-asm/images/runicon.png)
