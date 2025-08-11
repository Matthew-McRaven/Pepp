# Writing a machine language program

Writing a machine language program is a three-step process.

## Step 1.

With this Pep/9 application, you write the machine language program in hexadecimal in the Object Code pane.
The following figure shows the object program from Figure 4.43 of the text.

![fig0443](qrc:/help-asm/images/fig0443.png)

A hexadecimal program must follow precise formatting rules.
Each hex number representing a byte must contain exactly two characters.
Each character must be in 0..9, A..F, or a..f and must be followed by exactly one space.
There must be no leading spaces at the beginning of a line and no trailing spaces at the end of a line.
The last two characters in the file must be lowercase `zz`, which is used as the terminating sentinel by the loader.

## Step 2.

After writing the machine language program, load it by selecting Build→Load Object Code from the menu.

![load](qrc:/help-asm/images/load.png)

If there are no errors in the program a message to that effect will appear at the bottom of the main window.

![loadsucceeded](qrc:/help-asm/images/loadsucceeded.png)

## Step 3.

To execute the program select Build→Execute from the menu.

![execute](qrc:/help-asm/images/execute.png)

With this program, the output appears in the output pane.

![fig0441output](qrc:/help-asm/images/fig0441output.png)


# Tracing machine code

If you are debugging a machine language program that you wrote, the first step is to enter the program in hexadecimal in the Object Code pane.
The following figure shows the object program from Figure 4.35 of the text.
The input stream contains the two characters `up`.

![fig0435](qrc:/help-asm/images/fig0435.png) ![inputup](qrc:/help-asm/images/inputup.png)

Then select Debug→Start Debugging Object from the menu.
The Start Debugging Object action does three things — clears memory to all zeros, loads the machine language program into memory starting at address 0000, and puts you in the debugging mode.

![startdebuggingobject](qrc:/help-asm/images/startdebuggingobject.png)

At the beginning of the trace, the program counter (PC) is set to 0, indicating that the next instruction to be fetched is the one in memory at address 0000.
The memory dump highlights in blue the machine language instruction, in this example D1 FC15.

![fig0435trace1](qrc:/help-asm/images/fig0435trace1.png)

## Single step

To trace what happens when the instruction executes select Debug→Single Step from the menu or click the corresponding icon in the tool bar.

![singlestep](qrc:/help-asm/images/singlestep.png) ![singlestepicon](qrc:/help-asm/images/singlestepicon.png)

The debugger will perform one von Neumann cycle.
Namely, it will fetch the instruction, decode it, increment PC, and execute the instruction fetched.
In this example, the instruction D1 loads a byte from the input port FC15 into the accumulator.
Because `u` is the first character in the input stream, the instruction puts its hexadecimal equivalent 0x75 in the right half of the accumulator.
At this point, the CPU pane shows the instruction specifier D1 in binary and the operand specifier FC15 in hexadecimal.
This is the instruction that was fetched and executed.
The value of PC is the incremented value, which points to the instruction that will be executed next.
The memory dump highlights the instruction that will be executed next, not the instruction that was just executed.

![fig0435trace2](qrc:/help-asm/images/fig0435trace2.png)

The following figure shows the state of the machine after one more single step.
Instruction F1 0013 is the store byte instruction.
It stores the right half of the accumulator to memory location 0013.
The memory dump pane highlights in red that part of memory that was changed by the instruction.
The highlighted instruction in the memory dump, D1 FC15, is the instruction that will execute next.

![fig0435trace3](qrc:/help-asm/images/fig0435trace3.png)

If you want to terminate the debugging session without running the program to completion, select Debug→Stop Debugging from the menu or click the stop sign icon in the tool bar.

![stopdebugging](qrc:/help-asm/images/stopdebugging.png) ![stopdebuggingicon](qrc:/help-asm/images/stopdebuggingicon.png)
