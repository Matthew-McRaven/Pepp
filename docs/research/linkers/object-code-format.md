# Object Code Format

Elf

* allows arbitrary sections / data (and can write details about)
* stack trace
* I/O ports
* does not allow sections with overlapping addresses (implementation issue)

Custom

* more design work
* easier to write loader

Current thoughts:

* ELF allows me to bundle OS with user program, meaning terminal version can use the correct OS easily.
* ELF means I have to smash my object code into a pre-defined format.
* Custom object code format is novel design work (for which I already have a limited budget)
* Custom object code format may allow easy expression of simulator-specific data types
* ELF allows use of standard tools to dissect object code
* ELF is easy to link (once I write a linker)
