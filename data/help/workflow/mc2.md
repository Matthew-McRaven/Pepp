# Microcode Use

A microprogram consists of a sequence of microcode statements, which, when executed, implements a single ISA3 instruction.
Pep/9 CPU lets you write a microprogram code fragment in the Microcode pane and invoke the microassembler to translate the microprogram to binary, then execute the sequence of statements in the code fragment.
The IDE has a built-in unit test facility that specifies preconditions and postconditions to test the correctness of the code fragment.

Topics: [Writing a microprogram](#Writing), [Running a microprogram](#Running), [Unit tests](#Unit).

### Writing a microprogram

Three features of Pep/9 CPU help you write microprograms — automatic code generation, automatic un/commenting, and automatic formatting.

#### Automatic code generation

One way to write a microprogram is to simply enter the text in the Microcode pane.
However, you may prefer to enter a microcode statement with a minimum of typing.
Simply set the control signals for the statement with the input widgets in the CPU pane and then click the Copy to Microcode button.
For example, suppose you are at the following point writing your program.

[copycode1](../../../../../../help-micro/images/copycode1.png)

You could enter this text on the keyboard

A=6, B=7; MARCk

for the next cycle. However, instead of typing the text you can interactively make the settings in the CPU pane.
In the following figure, the user has entered 6 for control signal A, 7 for control signal B, and checked the MARCk check box.
The user can see that data is on the ABus and BBus because they are colored solid red, and that data from them will be clocked into the MAR because the line from the MARCk checkbox is solid black instead of gray.

[copycode2](../../../../../../help-micro/images/copycode2.png)

At this point, the user clicks the Copy to Microcode button.

[copycode3](../../../../../../help-micro/images/copycode3.png)

The effect is that the microcode text corresponding to the settings in the CPU pane is generated and inserted in the microcode pane as the following figure shows.

[copycode4](../../../../../../help-micro/images/copycode4.png)

#### Automatic un/commenting

The automatic un/commenting feature allows you comment and uncomment a group of contiguous lines of code.
For example, suppose you have entered the follwing lines of code in the Microcode pane.

[comment1](../../../../../../help-micro/images/comment1.png)

To comment the first two lines of code, first select them with the editor.
Any line that is partially selected will be affected by the operation.
In the following figure, the first line is completely selected and the second line is partially selected.
The second line will also be affected by the automatic comment operation.

[comment2](../../../../../../help-micro/images/comment2.png)

Now, select Edit→Un/Comment Line from the main menu, or press the equivalent keyboard shortcut for your platform.

[comment3](../../../../../../help-micro/images/comment3.png)

Any selected lines that are uncommented will be commented, and any selected lines that are commented will be uncommented.

[comment4](../../../../../../help-micro/images/comment4.png)

A handy feature of this tool is when a microprogram has more than one unit test.
In such cases, the first unit test is usually uncommented and the remaining tests are commented.
You can comment the first unit test and uncomment the second unit test in one operation by selecting _both_ tests.

[comment5](../../../../../../help-micro/images/comment5.png)

When you perform the operation, the first test is commented and the second test is uncommented.

[comment6](../../../../../../help-micro/images/comment6.png)

### Running a microprogram

After writing a microcode program in the Micorocode pane, there are two ways to execute the program.
You can run the program or you can debug it step by step.
See the section Debugging Use for how to debug your microprogram.

Suppose you have entered the following microcode in the Microcode pane.

[microcodeprogram](../../../../../../help-micro/images/microcodeprogram.png)

To run your program select Build→Run from the menu or click the corresponding Run icon on the toolbar.

[runmenu](../../../../../../help-micro/images/runmenu.png) [runicon](../../../../../../help-micro/images/runicon.png)

Pep/9 CPU will attempt to microassemble your microcode program.
If there are no errors, it will show the object code in the Object Code pane and execute the microcode.

[microcodeobject](../../../../../../help-micro/images/microcodeobject.png)

If you simply want to check for microcode errors without executing the program select Build→Build Microcode from the menu or click the corresponding Build Microcode icon on the toolbar.

[buildmenu](../../../../../../help-micro/images/buildmenu.png) [buildicon](../../../../../../help-micro/images/buildicon.png)

If there are errors, they will appear in red in the Microcode pane.
The following figure shows the error message that appears when a comma is omitted after the first control signal.

[errormessage](../../../../../../help-micro/images/errormessage.png)

If you would like, you can select Edit→Remove Error Messages from the menu to delete the error message.
Then, you can correct your error and try to run your program again.

[removeerrormessages](../../../../../../help-micro/images/removeerrormessages.png)

It is not necessary to remove the error message before correcting your program, as error messages are automatically removed when you rerun your program.

[Scroll to topics](#Topics).

### Unit tests

To establish a unit test for your program write the unit test precondition with the `UnitPre` keyword and the unit test postcondition with the `UnitPost` keyword at the beginning of a line and followed by a colon `:` .
The `UnitPre` statement clears all memory and CPU vaules to zero then sets values in memory and the CPU before the first microcode statement executes.
The `UnitPost` statement tests whether a memory cell or CPU register has a specified value after the last microcode statement executes.

If a unit test fails, a dialog alert is activated.

[postconditionerror](../../../../../../help-micro/images/postconditionerrordialog.png)

When you dismiss the dialog box, a detailed error message is displayed in the Microcode pane.

In the following figure, the programmer ommitted the CCk clock signal in the second microcode statement.
Consequently, the C bit was not clocked into its cell. The `UnitPost` statement specifies that C must be 1 at the conclusion of the unit test.
The error was triggerred because C was 0.

[postconditionerror](../../../../../../help-micro/images/postconditionerror.png)

There can be as many `UnitPre` and `UnitPost` statements as you like placed anywhere in the microprogram listing without affecting the unit test.
However, the convention is to place the `UnitPre` statements followed by the `UnitPost` statements before the first cycle statement in the microprogram.

#### Specifying register values

The instruction register IR is a three-byte register. If you specify

IR=0xa

the value stored in `UnitPre` or tested in `UnitPost` is

IR=0x00000a

The instruction register T1 is a one-byte register. If you specify

T1=0xabc

you will get the error message

// ERROR: Hexidecimal register value is out of range (0x00..0xFF).

All other registers in the register bank except IR and T1 are two-byte registers.
For example, if you specify `PC=0x12345` you will get an out-of-range message, and if you specify `PC=0x1` the value 0x0001 will be stored or tested.

The NZVC bits are one-bit registers. For example, `N=1` is a valid specification.
Although the specification `N=0` is technically not necessary in `UnitPre` because all register values are cleared to zero before applying unit preconditions, such a specification is valid.
The convention is to include it for emphasis.

#### Specifying memory values

Pep/9 main memory is byte addressable. For example, if you specify

Mem\[0x1234\]=0x5

the one-byte value 0x05 at memory address 0x1234 is set or tested.

It is frequently necessary to specify two adjacent bytes in memory, as with the following specification.

Mem\[0xabcd\]=0x11, Mem\[0xabce\]=0x22

You can specify a two-byte value for adjacent memory cells by writing a single value in the range 0x0100..0xffff.
For example, you can write the above specification as

Mem\[0xabcd\]=0x1122

[Scroll to topics](#Topics).
