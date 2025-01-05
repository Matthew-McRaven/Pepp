# Add instruction to set SP
* Status: Accepted
* Date: 2024-02-22
* Deciders: Stan, Matthew

## Issue
In Pep/9, there is no direct way to set the SP other than a context switch.
With Pep/10's dispatcher, the OS needs to be able to swap from the OS to the user stack before calling main.
MOVASP, which discards the existing SP, was added to avoid abusing context switches for this purpose.
If the programmer does not first MOVSPA and store A somewhere, the system will likely enter an inconsistent state during a context switch.


## Decision

Remove SWAPSPA and re-introduce MOVASP, which sets the stack pointer to the value of the accumulator.

## Previous Decision 

Add a SWAPSPA unary instruction which exchanges the accumulator and stack pointer.

## Constraints
In general, assembly language gives you powerful tools to crash your own programs (and the Pep/10 OS).
As students are programming at the machine-code level, we cannot ensure memory safety or termination **of user programs with respect to the simulated memory** without limiting hardware capabilities.
We don't want a supervisor/user privilege scheme, we must accept that the simulator may need to heuristically kill bad student programs (without causing errors in the simulating application).

This constricts the design space of the ISA + VM.
It does **not** mean that we should ignore the safety of the mechanisms and instructions we add.
Within our design space, some designs are safer/less complex/easier to teach than others.
For this problem, we should prefer designs which reduce the amount of prose required to explain the OS.

## Positions
Use non-immediates stack arithmetic
> This requires no additional mnemonics over Pep/9.
> We would compute the difference between SP and our desired new SP, and store it to a memory location.
> A stack arithmetic would take the stored value as an argument.
> This somewhat abuses SP arithmetic instructions, which traditional expect only immediate addressing.

Use a context switch to set SP
> This would not require any additional mnemonics over Pep/9.
> However, it could be error prone setting up the PCB, since the PCB is now stack-relative to the OS stack.
> This somewhat abuses the SRET instruction.

Keep using the MOVASP instruction
> This would save us from changing the current mnemonic map.
> However, the behavior of SWAPSPA is required, and it is imitated in the loader via a MOVSPA and STWA.
> This is a minimal increase in object code size over a SWAPSPA, and does not require introducing a SWAP class of mnemonics
> This does not require abuse of an instruction, but requires this instruction come with a disclaimer in the OS.

Add a SWAPSPA instruction to exchange A & SP
> This will reorganization of the opcode map to keep all MOV\* instructions together.
> It is the first instruction to swap values.
> Despite these problems, it is a safer default than MOVASP.

## Argument
Neither option using existing instructions is straightforward.

Since we have opcodes to spare, this is an easy option.

Between the two opcodes, MOVASP has the benefit of inertia, while SWAPSPA is safer-by-default.
Safe-by-default is generally a good design strategy.
While SWAPSPA is not perfectly safe (you could still clear A), it is an improvement over the current implementation.

That being said, we ultimately have decided to use MOVSPA because SWAPSPA introduces unnecessary asymmetry into the instruction set.

## Implications
We have one fewer opcode available for future extensions.
