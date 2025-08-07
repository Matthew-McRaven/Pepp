### The memory dump

View the Memory Dump pane by selecting View→Code/CPU/Memory from the menu or by clicking the corresponding icon in the tool bar.

![codecpumemory](images/codecpumemory.png) ![codecpumemoryicon](images/codecpumemoryicon.png)

The Pep/9 application may run noticibly slower when the Memory Dump pane is visible.
You can click either of the other two view icons to hide the Memory Dump pane.

![codecodememory](images/codecodememory.png)

Each row of the memory dump pane displays eight contiguous bytes of memory and has three parts.

![memorydump](images/memorydump.png)

The first part shows the address of the first byte in the row.
The second part shows a list of eight bytes, each one displayed as two hexadecimal digits.
In the above figure, 0008 is the address of 0E, the first byte of the row.
Byte F1 is at address 0009, byte FC is at address 000A, and so on.

The third part in the memory dump pane shows the ASCII representations of the eight bytes in the row.
Some bytes in a memory dump are generated from ASCII strings, and are meaningeful when interpreted as such.
For example, in the above figure bytes 48 and 69 at addresses 000D and 000E are displayed in the third part as letters `H` and `i`.
Most bytes in a memory dump are not generated from ASCII strings, and are meaningless when interpreted as such.
In the above figure, byte F1 is displayed as an accented n. When a byte represents a nonprintable ASCII control character, it is shown as a period.

You can scroll to any location in memory using the scroll bar on the right.
To scroll to a specific memory location enter the address in hexadecimal in the input field at the bottom of the pane.
Click the SP button to scroll to the region of memory pointed to by the stack pointer.
Click the PC button to scroll to the region of memory pointed to by the program counter.

![fig0434](qrc:/help-asm/images/scrollto.png)
