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
