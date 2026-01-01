# Rename `Microcode 2` to `Microarchitecture 2`
* Status: Accepted
* Date: 2025-12-31
* Deciders: Stan, Matthew

## Issue
The Microcode 2 level of abstraction should be Microarchitecture 2 so that it applies to both RISC and CISC machines.

## Decision
We will go forward with this renaming.

## Argument
In this view, LG1 is focused on creating blocks that have well-defined inputs and outputs. 
At microarchitecture level, you assemble those basic LG1 blocks to implement the instruction set architecture of the computer.

You can reuse the same LG1 blocks to create different microarchitectures which implement the same ISA.
e.g., on a RISC-V vector core, you could create a single ALU at the LG1 level and stamp down different numbers of that ALU depending on the desired performance of the target.
All configurations implement the same ISA.

At the microarchitecture 2 level, CISC computers can be programmed (microcode) at runtime.
RISC computers are composed of fixed-function blocks whose control flow is fixed at design time.

## Implications
Chapter 1, and chapters 11-12 will need their references to the "microcode" level of abstraction updated.
Any interface which references Mc2 will need to be updated in the help and welcome screens.
Enumerations referring to Mc2 are now out-of-date.
