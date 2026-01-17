# Replace two-byte bus, MIPS in CS6E
* Status: Accepted
* Date: 2022-02-22
* Deciders: Stan, Matthew

## Issue
In CS5E, we teach a two-byte bus variant of the Pep/9 CPU.
We also teach a single RISC processor, MIPS.
While we have a simulator for the two-byte bus, we do not have a simulator for MIPs.
It would be a significant improvement to give students hands-on experience with a RISC processor with a GUI simulator.

## Decision
We will teach [RISC-V](https://riscv.org) as our RISC architecture, replacing MIPS.
Additional expository text is needed to allow students to write meaningful programs, so content must be cut with respect to CS5E.
We will reclaim some space by removing exposition regarding the two-byte.

## Constraints
The purpose of the two-byte bus was to teach students about a one (simple) optimization to improve CPI.

We would also like to expose the impacts of caching to students.
The previous pep9suite includes a caching simulator, but no text describes its usage.

Since RISC processors continue to rise in popularity, we must cover them to some extent in our text.

## Positions on RISC architecture
Keep MIPs as RISC architecture
> MIPs is no longer a widely popular architecture.
> Many MIPs implementations have artifiacts that are complex to teach, such as branch delay slots.
> We currently lack an ISA simulator for MIPs, and there are not many OS implementations to fork.
> It is likely we would be able to find a OS LG1 implementation.

Switch to ARM from MIPs
> A widely deployed instruction set that is proprietary.
> We would have to choose which ARM instruction set we target (e.g., ARMv7, arch64), which implies a bit-width decision.
> Students would be very likely to encounter this ISA in future programming endeavours.
> Depending on which family we target, we may have to cut a large number of instructions from our implementation.
> We currently lack an ISA simulator for any ARM family, and there are not many OS implementations to fork.
> I doubt we will find an OS LG1 implementation.

Switch to RISC-V from MIPs
> Students may be less likely to encounter in real-world application programming.
> While they are becoming more popular in the embedded space, they aren't common in the mobile, laptop, desktop, or server space.
> The instruction set is modular, meaning we can reasonably implement the full base ISA.
> Being an open-source architecture, we can borrow existing LG1 designs for Pep10CPU.
> There are multiple OS ISA simulators whose implementations we can fork.

## Positions on removal of two-byte bus
The following discussion assumes RISC-V as our RISC architecture, but the arguments remain the same for any choice of RISC.

Keep the two-byte bus
> Reduces the amount of new material to write in CS6E.
> Does not requiring writing a RISC-V simulator.

Remove two-byte bus, teach RISC-V MA2 emulation of Pep/10 CPU
> Need to determine mapping of RISC-V:Pep/10 registers at the MA2 level.
> Students would be taught to implement Pep/10 CPU using RISC-V instructions as a form of microcode.
> This establishes continuity between the two halves of chapter 12.
> This would require some kind of graphical data & control path to maintain parity to Pep10CPU within the application.
> A major downside is that  32-bit registers would be badly contorted to do 8-bit operations.
> This implies the implementation/exercises either: 1) ignore 8=>32 bit coercion , or 2) insert &0xFF operations after performing each arithmetic operation.
> Additionally, there are no native status bits, so we would have to come up with a bespoke solution (custom instructions?).

Remove two-byte bus, teach RISC-V ISA3 emulation of Pep/10
> Need to determine data structures in RISC-V to enable the Pep/10 VM (e.g., register bank, memory).
> Students would be taught to use the datastructures and von-Neumann loop of the emulator to implement the RTL of Pep/10.
> Emphasis must be placed on the integration of student code fragments into the emulation framework
> Complexity arises from mapping Pep/10 registers to memory locations, but is required to persist register values across subroutines.
> Alternatively, we could stray from the standard register naming conventions and use dedicated RISC-V registers to hold Pep/10 values, which implies the same masking issues as above.

Remove two-byte bus, teach translations from C to RISC-V
> Exposition on two-byte bus must be removed to provide additional room in text for discussing details of RISC-V
> Students would be taught to translate simple C fragments to RISC-V, including: math expressions, assignment expressions, and basic control flow.
> This would replicate content we teach at Pepperdine in Computer Systems, with Pep/10 replaced by RISC-V

## Argument
As an open-source software product, we benefit from re-using the works of others (with attribution).
RISC-V has open source tools for simulation and verification, in addition to HDL implementations.
While tools and implementations exist for other architectures, philasophically it is reasonable for us to work with RISC-V.

From an implementation perspective, the RV32I base integer instruction set is minimal compared to other available choices.
This means we can contribute an entire RV32I graphical simulator, rather than some partial implementation of another instruction set.

For teaching, RV32I features a small set of instructions which is not vastly different than Pep/10 (other than instructions being register-to-register).
Additionally, we avoid complexity of branch delays in MIPS.

For these reasons, we've settled on RISC-V.



While ISA emulation is an interesting topic (e.g., Roseta on MacOS), it is probably too complex to teach undergraduates.
This eliminates two of the three alternatives to the two-byte bus.

The two-byte bus allowed students to see the impacts of microarchitecture performance optimizations on implementation.
The microcode they wrote varied significantly from the one-byte bus version.

At the end of chapter 12, students learned about MIPS, with on-paper assignments due to the lack of us shipping a GUI simulator.
If we switch to RISC-V and omit the two-byte bus, we still have opportunities to show performance-enhancing designs using RISC-V.
For example, we can integrate our cache simulator fully into the RISC-V simulator, and students can have assignments manipulating the configuration to improve perf.

## Implications
We still need to provide a two-byte Pep/9 implementation, so it has not been entirely eliminated.

With the addition of RISC-V, we will need to add full support for a second assembler / ISA simulator to our application package.
