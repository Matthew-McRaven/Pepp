# New Application Modes
Several new modes would be beneficial to Pepp

## Test mode
Mode which lets you author pre- and post-conditions, and run the source program aganst all the tests at once.
Should be available in all ISA3, ASMB and MA projects

### Queries
I would like some ability to write temporal queries over programs
For example, `after calling main, the PC did not reenter the OS` is requirement as ASMB3/ISA3

## Performance mode
Similar to the stats mode of the previous version.
Ideally it would work like tracy, where you could see performance counters for individual functions or call stacks.

## Interactive tutorials
We should be able to have an interactive UI tutorial for each level of abstraction rather than referring people to the help docs.

# New Pep/X Asmb Features
Pep/X is my standin for the language/simulator extension features beyond what is included in the 6th edition of the book

# New UI elements
## Cache View
We already have a nice cache simulator in Pep9, which we should bring forward to Pep/10 and RISC-V
## Better Main-memory explorer
RISC-V has 32bits of address space.
Our little table can't handle that many items.
## Disk explorer
As I do more useful things with Pepp, persistence is important.
A UI to explore disk contents is helpful

# New Pep/X ISA Features
## Interrupts
All instructions in Pep/10 always succed.
This will not be true as I extend the ISA.

Interrupts are the classic method for error handling and signaling.

This likely involves a vectored interrupt service routine handler, and an ABI to pass data into ISRs.
## Virtual Memory
64k is a big limit for writing useful programs.
### Segmented Memory
A flat 32-bit address space would be a full rework of Pep ISA, so using segment regs like x86 is a good first step
These segments should support memory protections (RWX) even if there is no supervisor mode
### Paged-segmented
With an OS to manage resources, I don't want to operate directly on physical memory.

# New Pep/X MA2 Features
## Allow direct simulation of RTL
Gives me a golden model to compare against.
That RTL can be displayed in the help system.

As I add new instructions / features, the RTL sim should be quite helpful

## RTL to microcode synthesis
Feedback I received from my Pep9Milli paper in grad school was to explore RTL to microcode synthesis.
With an RTL simulator AND a microcode simulator, would be fairly easy to verify equivalence.

## L1 cache and beyond
As I try to improve peformance, I expect I will want at minimum an L1 cache, but likely multi-level

## TLB
If I get paging, I will want a translation lookaside buffer

## Multiple Pep/X harts
Allows limited HW concurrency. Expensive FUs can be duplicated.

## OoO, speculation, etc
Drive up perf, may be necessary depending 

# Implement Pep/10 in HDL
Ultimately, I want a business-card sized circuit board with an FPGA and display on it.
The FPGA will contain a Pep/10 CPU which will drive the display.
The display will contain my resume, and can be navigated with buttons.

To implement this, I need a HDL implementation of Pep/10.
I made significant progress

# Run Doom on Pep/10
Ideally using the same HW described above.
Makes for a nice, flashy demo and a good write-up about how to organically grow a small core into something more usable.

As part of this effort, I probably want to have a decent collection of benchmarks which I can run against my CPU.


# New project types
## C IDE
Would like to be able to debug chapter 2 programs inside of the Pepp IDE
### Multi-file, multi-abstraction projects
Most useful C is not a single file, and our current project model does not support multi-file projects.
Additionally, each file is going to have special interactions with the simulator and debugger.
e.g., the c source files
### C Compiler
Need a C compiler that outputs (RISC-V, Pep/10) assemly code and output relevant debug info.
## Logic Gate Simulator
Allow editing and simulating LG1 and MA2 projects.
Allows inspecting truth tables, etc

# Software Architecture Improvements
## Port to Tablets
Bring (a subset) of the application modes to a tablet and mobile.
This will require rethinking many UI elements.

The decision of which UI to use is a runtime decision -- the WASM target should be able to splat either UI.

## Deploy to Windows on Arm, Linux on ARM
While we have native ARM builds for Mac OS, we have no such builds on Windows or Linux.
Both are officially supported by QT these days (currently Qt 6.10).
I anticipate both targets will see increased adoption

## Implement per-commit UI unit tests
Should have all common workflows unit/regression tested via GUI.

## Expose unit testing to Python
Should be able to assemble/compile/run Pepp programs at various levels of abstraction.
Basically a python-integrated version of pepp-term.

## Self-hosting assembler/compiler
Would be nice if the pepp compiler and assembler are written in C, and could therefore target Pep/X.