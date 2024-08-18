
$ Writing Programs

Pep/9 is a virtual machine for writing machine language and assembly language programs.
It is designed to be used with the textbook, _Computer Systems_, J. Stanley Warford, Fifth edition, Jones and Bartlett Learning, 2017.
The goal of the book is to teach the fundamentals of the classic von Neumann machine.
You should use the Pep/9 system in conjunction with the textbook to write machine language and assembly language programs.

Topics: [Viewing panels](#Viewing), [Input and output](#Input), [Endless loops](#Endless), [Textbook examples](#Examples), [File extensions](#File).

### Viewing panels

The Pep/9 system has three panels — the code panel, the CPU panel, and the memory dump panel.
You can change which of these panels is visible by making the appropriate selection from the View menu or clicking the corresponding icon on the tool bar.

![codecpumemory](qrc:/help-asm/images/codecpumemory.png) ![viewicon](qrc:/help-asm/images/viewicon.png)

When you click in one of the panes inside a panel it becomes the active pane, which is indicated by the blue color of its label at the top of the pane.
If you double click on the label the pane will expand to its maximum height.
The following screenshot shows the label at the top of the Source Code pane in the code panel.

![viewicon](qrc:/help-asm/images/sourcecode.png)

[Scroll to topics](#Topics).

### Input and output

The top part of the CPU panel shows the content of the Pep/9 central processing unit.
The bottom part of the panel shows the input and output of your machine language or assembly language program.
The Pep/9 system supports both batch and interactive I/O, which you can select by clicking the appropriate tab.

![iotab](qrc:/help-asm/images/iotab.png)

When you execute in batch mode, you must enter the input in the Input pane before you run your program.
The output will appear in the Output pane as the program executes.

Your program executes in interactive mode when you select the Terminal I/O tab.
In this mode, executing an input statement in your program will make the program pause and wait for you to enter the input in the Input/Output pane.
The executing program accepts your input and continues executing when you press the enter or return key.

[Scroll to topics](#Topics).

### Endless loops

If you execute a program with an endless loop, you can interrupt it by pressing <command>+`.` on a Mac or <control>+`.` on a Windows computer.
Or, you can select Debug→Interrupt Execution from the menu.
Either of these actions will pause execution of the program, allow you to use the debugging tools, and, if you wish, continue execution of your program.

![interruptexecution](qrc:/help-asm/images/interruptexecution.png)

Alternatively, you can terminate execution of your program by selecting Debug→Stop Debugging from the menu or clicking the Stop icon in the tool bar.

![stopdebugging](qrc:/help-asm/images/stopdebugging.png) ![stopdebuggingicon](qrc:/help-asm/images/stopdebuggingicon.png)

[Scroll to topics](#Topics).

### Textbook examples

All of the programs in the figures of the textbook are available in this help system.
To try a program out select it from the column on the left and click the Copy to Source button.

![copytosource](qrc:/help-asm/images/copytosource.png)

You can then run it or trace it as described in later sections of this help system.

[Scroll to topics](#Topics).

### File extensions

The Pep/9 system uses the following file extensions:

`.pep` is the file extension for source code programs.  
`.pepo` is the file extension for object code programs.  
`.pepl` is the file extension for the formatted program listing.

All these files are text files that you can modify with your favorite text editor or word processor outside the Pep/9 application.
When you open a `.pep` file, it opens in the Source Code pane.
When you open a `.pepo` file, it opens in the Object Code pane.
Although you can save a `.pepl` file, there is no facility for inputting it into the Pep/9 application.

[Scroll to topics](#Topics).
