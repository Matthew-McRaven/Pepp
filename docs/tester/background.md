# Background

## Programming

* [Machine code](https://en.wikipedia.org/wiki/Machine\_code) is the raw bytes executed by a CPU; pain for humans to write
* [Assembly](https://en.wikipedia.org/wiki/Assembly\_language) is a text representation of machine code; easy for humans to write
* Programs called [assemblers ](https://en.wikipedia.org/wiki/Assembly\_language#Assembler)translates that text to machine code
* High-level languages (like C++) are translated by a compiler to assembly, and then translated to machine code

## CPUs

* Executes sequential bytes of machine code (instructions) each cycle
* Clock speed (e.g., 4GHz) refers to the number of cycles/second
* [Registers](https://en.wikipedia.org/wiki/Processor\_register) are fast (access time: <1 cycle), named values easily accessible in machine code. Extremely limited number (<32 per core)
* The bitness (e.g., 64-bit) usually refers to the number of bits in a normal register
* [Main memory](https://en.wikipedia.org/wiki/Computer\_memory) is slow (access time: 100s-10,000s cycles) and often difficult to access in machine code. Often 8-128GB total in a computer
* An [Instruction Set Architecture](https://en.wikipedia.org/wiki/Instruction\_set\_architecture) (ISA) is an abstract model of a CPU describing supported instructions, available registers, and interactions with main memory E.g., [x86-64](https://en.wikipedia.org/wiki/X86-64) is the ISA Intel implements across its different concrete CPU hardware implementations
* Assembly language targets a particular ISA, not a specific CPU
* [Complex Instruction sets](https://en.wikipedia.org/wiki/Complex\_instruction\_set\_computer) (CISC) perform many operations per instruction, variable byte count / instruction
  * For example: Load into register _A_ from memory address in register _X_, multiple _A_ by _B_, then add _C_
* [Reduced Instruction sets ](https://en.wikipedia.org/wiki/Reduced\_instruction\_set\_computer)(RISC) perform one operation per instruction, fixed byte count / instruction
  * For Example: Add register A to B
