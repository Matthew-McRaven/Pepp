# Symbolic trace

The symbolic trace feature is controlled by the trace tags described in the text.
Trace tags are contained in assembly language comments and have no effect on generated object code.
Each trace tag begins with the `#` character and supplies information to the symbol tracer on how to format and label the memory cell in the trace window.

If your program does not include trace tags, the memory trace pane will not be visible when you run your program.
You can still use the debugger to single-step through your program and set break points.
If it does include trace tags, but errors are detected in the tags, a blue warning will be issued.
You can still run your program and use the debugger, but the memory trace pane will not be visible.
The screenshot below shows what happens if you make an error in one of the trace tags in the program from Figure 5.27.

![fig0527tracetagwarning](qrc:/help-asm/images/fig0527tracetagwarning.png)

If your program does include trace tags and they have no errors, the memory trace will automatically become visible below the listing trace pane.
As you single-step through the program, you can see the global variables on the left, and the run-time stack on the right.
Here is a screenshot of the memory trace pane from the program in Figure 6.21.
The memory cell for `k` on top of the run-time stack is colored red because a `STWA` instruction has just changed its value.

![memorytrace](qrc:/help-asm/images/memorytrace.png)

The sybolic trace feature also displays the fields of a global `struct`, and storage from the heap allocated with a call to `malloc`.
Here is a screenshot of the memory trace pane from the program in Figure 6.48.
The stack on the left contains the local variables, and the heap on the right contains the dynamically allocated variables.
The program has just executed a call to `malloc`, which has allocated a new node shaded green with fields `data` and `next`.

![memorytrace2](qrc:/help-asm/images/memorytrace2.png)

See the textbook for information on how to use trace tags in your programs.


# Trace tag warnings

If your program assembles correctly but there is an error in your trace tags, a trace tag warning is displayed.
For example, here is what happens if you make an error in one of the trace tags in the program from Figure 5.27.

![fig0527tracetagwarning](qrc:/help-asm/images/fig0527tracetagwarning.png)

Programs with trace tag warnings have assembled correctly and will still run.
In the debugger, you can still single step through the program, set break points, and trace memory in the Memory Dump pane.
However, the symbolic trace feature that shows the C memory model graphically in the Memory Trace pane is disabled until you fix the trace tag error and reassemble your program.
