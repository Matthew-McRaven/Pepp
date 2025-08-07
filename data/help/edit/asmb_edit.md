# Writing an assembly language program

Writing an assembly language program is a four-step process.

## Step 1.

With this Pep/9 application, you write the assembly language program using the built-in text editor in the Source Code pane.
The following figure shows the source program after entering the program from Figure 5.19 of the textbook.

![fig0519](qrc:/help-asm/images/fig0519.png)

Strict formatting is not required as long as there is one instruction or dot command per line, and at least one space between a mnemonic and an operand specifier.
For example, the above program could be entered as follows.

![fig0519unformatted](qrc:/help-asm/images/fig0519unformatted.png)

## Step 2.

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

## Step 3.

Load your program by selecting Build→Load Object Code from the menu.

![load](qrc:/help-asm/images/load.png)

## Step 4.

To execute the program select Build→Execute from the menu.

![execute](qrc:/help-asm/images/execute.png)
