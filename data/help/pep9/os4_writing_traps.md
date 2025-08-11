# Writing Trap handlers

To modify the operating system for the problems in Chapter 8 of the text is a six-step process.

## Step 1.

Decide on your mnemonic for your new instruction. It will replace one of `NOP0`, `NOP1`, `NOP`, `DECI`, `DECO`, `HEXO`, or `STRO`.
Select the menu option System→Redefine Mnemonics to change the mnemonics of one of the instructions.

![redefinemnemonicsmenu](../../../../../../help-asm/images/redefinemnemonicsmenu.png)

The dialog box requires you to enter a mnemonic and its allowed addressing modes if it is nonunary.
For example, change the mnemonic for the unary instruction `NOP0` to `ECHO`.

![redefinemnemonics](../../../../../../help-asm/images/redefinemnemonics.png)

## Step 2.

In this Help system in the pane on the left, select Pep/9 Operating System, and then click the Copy to Source button.
The default operating system will be copied to the Source Code pane.

![pep9oshelpsystem](../../../../../../help-asm/images/pep9oshelpsystem.png)

## Step 3.

Modify the trap handler part of the operating system to implement your new instruction.
As an example, here is the original NOP0 trap handler.

![nop0](../../../../../../help-asm/images/nop0.png)

And here is how you would modify it to implement the new `ECHO` instruction in place of `NOP0`.

![echo](../../../../../../help-asm/images/echo.png)

CAUTION: You cannot use any trap instructions in your trap handler.

## Step 4.

Select System→Assemble & Install New OS to assemble and install the reprogrammed operating system.

![assembleinstallnewos](../../../../../../help-asm/images/assembleinstallnewos.png)

A message will inform you if your modified operating system assembled correctly.

![osinstalledmsg](../../../../../../help-asm/images/osinstalledmsg.png)

You can save your modified operating system as a `.pep` file as you would any other Pep/9 assembly language program.

## Step 5.

Write an assembly language program to test your new instruction with the new mnemonic.
The assembler should recognize the new mnemonic and generate the appropriate object code.
For example, your test program might be the following.

![echoprogram](../../../../../../help-asm/images/echoprogram.png)

## Step 6.

Run or Start Debugging your program written in step 5.
In this example, whatever character is placed in the Input pane should be echoed in the output pane.

![pecho](../../../../../../help-asm/images/pecho.png)
