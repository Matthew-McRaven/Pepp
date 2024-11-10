# About Pep/10

* Pep/10 is the new architecture for upcoming Computer Systems, 6th edition (CS6E), with some major changes over [Pep/9](https://github.com/StanWarford/pep9suite) and [Computer Systems, 5th edition](https://computersystemsbook.com/)
  * Both have companion software to allow students to write & debug assembly language programs
  * CS6E will come with a [new IDE](https://github.com/Matthew-McRaven/Pepp) running on Win/Mac/Linux/[WASM](https://compsys-pep.com/), containing tools for Pep/10, Pep/9, and Pep/8; I can finally stop maintaining CS4/5E software
* 16-bit CISC ISA used to teach computer architecture & assembly language to undergraduates
  * Designed for easy hand translation from C to assembly
* [Accumulator machine](https://en.m.wikipedia.org/wiki/Accumulator\_\(computing\)#Accumulator\_machines) with 1 general purpose register and 5 special registers. See register descriptions in the Registers section
* Unary instructions do not access memory and only operate directly on registers
* Non-unary instructions contain a extra 16-bit operand, which is interpreted using one of 8 addressing modes
* With only 1 GP register, `C` operations like `B, A = A, B` need extra temporary variables in memory
* Uses [memory-mapped IO](https://en.wikipedia.org/wiki/Memory-mapped\_I/O\_and\_port-mapped\_I/O)
  * Read and write specific memory addresses for stdin, stdout rather than accessing files

## Registers

| Register              | Short Name | Special or General? | Usage                                                                                                                      |
| --------------------- | ---------- | ------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| Accumulator           | `A`        | General             | Destination register for most instructions                                                                                 |
| Index                 | `X`        | General-ish         | Offsets for some addressing modes; alternative destination register                                                        |
| Stack Pointer         | `SP`       | Special             | Contains the current stack pointer, implicitly modified by instructions `CALL` and `RET`                                   |
| Program Counter       | `PC`       | Special             | Contains the address of the next instruction to execute. Modified with control flow mnemonics like `BR`, `CALL`, and `RET` |
| Instruction Specifier | `IS`       | Special             | Contains the current opcode. Not accessible by the user                                                                    |
| Operand Specifier     | `OS`       | Special             | Contains the opcode's argument if it is non-unary. Not accessible by the user                                              |

## Addressing Modes

| Addressing Mode         | `aaa`-field | `a`-field | Letters | Operand                 | Commonly used to Access...                                         |
| ----------------------- | ----------- | --------- | ------- | ----------------------- | ------------------------------------------------------------------ |
| Immediate               | `000`       | `0`       | i       | OS                      | Constants                                                          |
| Direct                  | `001`       |           | d       | Mem\[OS]                | Global variables                                                   |
| Indirect                | `010`       |           | n       | Mem\[Mem\[OS]]          | Items pointed to with a global pointer                             |
| Stack-relative          | `011`       |           | s       | Mem\[SP + OS]           | Variables allocated on the stack                                   |
| Stack-relative deferred | `100`       |           | sf      | Mem\[Mem\[SP + OS]]     | Items pointed to with stack-allocated pointers                     |
| Indexed                 | `101`       | `1`       | x       | Mem\[OS + X]            | Items in global arrays                                             |
| Stack-Indexed           | `110`       |           | sx      | Mem\[SP + OS + X]       | Fields in stack-allocated structs, items in stack-allocated arrays |
| Stack-deferred indexed  | `111`       |           | sfx     | Mem\[Mem\[SP + OS] + X] | Fields in struct pointed to by a stack-allocated pointer           |
