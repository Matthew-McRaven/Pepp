# Break points

If you want to trace a program up to a specific instruction and you do not want to single step through all the previous statements to get there, you can set a break point at the instruction and then Continue Debugging to that point.
For example, the following figure shows the program from Figure 6.10 in the textbook after Start Debugging.
The input stream is `Hello, world!*`.

![fig0610trace1](qrc:/help-asm/images/fig0610trace1.png) ![fig0610trace1mem](qrc:/help-asm/images/fig0610trace1mem.png)

The gray strip on the left of the listing is the break point area.
If you click in the break point area next to an executable statement, a red circle will appear to indicate that the statement has a break point.
In the above figure, break points have been set for the `LDBA` instruction at 000A and the `LDBA` instruction at 0019.
To remove a break point click the red circle.

To execute all the statements up to the next break point, select Debug→Continue Debugging from the menu or click on the corresponding icon in the tool bar.

![continuedebugging](qrc:/help-asm/images/continuedebugging.png) ![continuedebuggingicon](qrc:/help-asm/images/continuedebuggingicon.png)

The following figure shows that all the statements up to but not including the one at 000A have executed.
The previous instruction `STBA` put the letter `H` in the global variable `letter` at 0003.
At this point, the statement to be executed is the statement at the top of a `while` loop.

![fig0610trace2](qrc:/help-asm/images/fig0610trace2.png) ![fig0610trace2mem](qrc:/help-asm/images/fig0610trace2mem.png)

The next figure shows the debugger after one more Continue Debugging operation.
The debugger executed the statements in the body of the `while` loop.
The debugger did not stop at the second break point because that break point is on an instruction in a nested `if` statement that did not execute.
The previous statement that executed was the `BR` instruction at 002B.
The memory trace shows the value of `e` stored to `letter` by the `STBA` instruction at 0028.

![fig0610trace2](qrc:/help-asm/images/fig0610trace2.png) ![fig0610trace3mem](qrc:/help-asm/images/fig0610trace3mem.png)
