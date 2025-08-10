### Memory-mapped I/O

In normal operation, the <code>STBA charOut,d</code> instruction outputs a character to the output port and the <code>LDBA charIn,d</code> instruction inputs a character from the input port.
In the event that <code>STBA charIn,d</code> executes, which attempts to store to the input port, the value stored will be visible in the Memory Dump pane.
However, that value will not be readable because a subsequent load will get its value from the input stream.
In the event that <code>LDBA charOut,d</code> executes, which attempts to load from the output port, the value loaded will be the most recent character stored to the output port.
