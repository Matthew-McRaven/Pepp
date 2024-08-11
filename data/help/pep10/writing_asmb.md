Pep/10: Assembly Language Documentation code 

Assembly Language

Reference: _Computer Systems_, Chapters 5, 6.

An assembly language program is a sequence of lines, each line containing either an instruction from the Pep/9 instruction set specfied by a mnemonic, or a pseudo-operation, also called a pseudo-op or dot command.
The assembly language program must end with the pseudo-op `.END`.

Topics: [Writing an assembly language program](#Writing), [The Run Source option](#Run), [Trace tag warnings](#Trace).

### Writing an assembly language program

Writing an assembly language program is a four-step process.

#### Step 1.

With this Pep/9 application, you write the assembly language program using the built-in text editor in the Source Code pane.
The following figure shows the source program after entering the program from Figure 5.19 of the textbook.

![fig0519](qrc:/help-asm/images/fig0519.png)

Strict formatting is not required as long as there is one instruction or dot command per line, and at least one space between a mnemonic and an operand specifier.
For example, the above program could be entered as follows.

![fig0519unformatted](qrc:/help-asm/images/fig0519unformatted.png)

#### Step 2.

After writing the assembly language program, assemble it by selecting Build→Assemble Source from the menu or clicking on the hammer icon in the tool bar.

![assemble](qrc:/help-asm/images/assemble.png) ![assembleicon](qrc:/help-asm/images/assembleicon.png)

If there are no errors in the program a message to that effect will appear at the bottom of the main window.

![assemblysucceeded](qrc:/help-asm/images/assemblysucceeded.png)

The translated program will appear in the Object Code pane, and a formatted listing will appear in the Assembler Listing pane.

![fig0519assembled](qrc:/help-asm/images/fig0519assembled.png)

If there is an error in your program, an error message will appear in the source code pane.
The following screenshot shows what would happen if you fail to supply the addressing mode for the `STRO` instruction.

![fig0519error](qrc:/help-asm/images/fig0519error.png)

If you would like, you can select Edit→Remove Error Messages from the menu to delete the error message.
Then, you can correct your error and try to assemble your program again.
It is not necessary to remove the error message before correcting your program, as error messages are automatically removed when you reassemble your program.

![removeerrormessages](qrc:/help-asm/images/removeerrormessages.png)

If you want to format your source code select Edit→Assemble & Format from the menu.
Your source code will be assembled and replaced with the standard formatted source code as it appears in the Assembler Listing pane.

![formatfromlisting](qrc:/help-asm/images/formatfromlisting.png)

#### Step 3.

Load your program by selecting Build→Load Object Code from the menu.

![load](qrc:/help-asm/images/load.png)

#### Step 4.

To execute the program select Build→Execute from the menu.

![execute](qrc:/help-asm/images/execute.png)

[Scroll to topics](#Topics).

### The Run Source option

The three steps — Assemble, Load, Execute — are combined in the single option called Run Source.
You can initiate a run by selecting Build→Run Source from the menu or by clicking on the Run Source icon in the tool bar.

![runsource](qrc:/help-asm/images/runsource.png) ![runicon](qrc:/help-asm/images/runicon.png)

[Scroll to topics](#Topics).

### Trace tag warnings

If your program assembles correctly but there is an error in your trace tags, a trace tag warning is displayed.
For example, here is what happens if you make an error in one of the trace tags in the program from Figure 5.27.

![fig0527tracetagwarning](qrc:/help-asm/images/fig0527tracetagwarning.png)

Programs with trace tag warnings have assembled correctly and will still run. In the debugger, you can still single step through the program, set break points, and trace memory in the Memory Dump pane. However, the symbolic trace feature that shows the C memory model graphically in the Memory Trace pane is disabled until you fix the trace tag error and reassemble your program.

[Scroll to topics](#Topics).
