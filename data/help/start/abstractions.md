# Levels of Abstraction
Pep/10 supports multiple levels of abstraction described in the [Computer Systems textbook](), each providing a different view of the computing process.
Each level uses different workflows and user interface elements for writing and debugging programs.
[Evil level figure]()

## Supported Levels
While Computer Systems, 6th edition describes 7 levels of abstraction, only the following levels are supported in this IDE.

| Abbreviation | Name                                 | Described in chapters | Notes                                                                                                                                                          |
| ------------ | ------------------------------------ | ----------------------| -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Mc2**      | Micro Code                           | 11                    | Microcode controls CPU’s datapath; used to implement the fetch–execute cycle and define instruction behavior at the hardware control level.                    |
| **ISA3**     | Instruction Set Architecture         | 4                     | Machine instructions written in hexadecimal object code, focusing on instruction semantics without binary encoding details.                                    |
| **Asmb3**    | Assembly Language, bare metal        | 5, 12                 | “Bare metal” mode without operating system support; teaches symbols and macros before system calls.                                                            |
| **OS4**      | Operating System                     | 8                     | Introduces system calls and trap handling; shows OS mediation between user programs and hardware.                                                              |
| **Asmb5**    | Assembly Language, with full OS      | 6                     | Assembly programs using system calls; may be generated from high-level languages like C, emphasizing translation and runtime services.                         |

## Supported Architectures
This IDE is designed from the ground up to support Pep/10, the architecture introduced in the 6th edition of Computer Systems, and is the recommended architecture for new projects.

For backwards compatibility, both Pep/9 and Pep/8 remain available. 
These earlier architectures retain their original instruction sets and memory layouts, ensuring that existing course materials and example programs can still be used without modification.

| Architecture | Textbook Edition | Notes                                                                                           |
| ------------ | ---------------- | ------------------------------------------------------------------------------------------------|
| **Pep/10**   | 6th Edition      | Recommended for new projects, includeng a powerful macro assembler and regular instruction set. |
| **Pep/9**    | 5th Edition      | Introduces memory-mapped input/output                                                           |
| **Pep/8**    | 4th Edition      | Legacy architecture with reduced instruction set and earlier toolset support.                   |
