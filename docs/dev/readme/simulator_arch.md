# Simulator Architecture Decisions
* Status: Accepted
* Date: 2026-06-08
* Deciders: Matthew

## Issue
With the revisions to the RISC-V chapters, we need to extend the RISC-V simulator to support the same set of memory-mapped operations as Pep/10.
I would like to refactor the RISC-V simulator to share the same APIs as Pep/10 rather than be its own parallel beast.
However, there's several problems I've encountered trying to make these adjustments.

### Mismatched Buse Address Sizes
There is currently a split between 16-bit and 32-bit simulators due to parameterization on the address type.
This prevents intermixing of RISC-V and Pep CPUs within the same system type.
Non-unified system types have the downstream effect of forcing a hard split in project types -- one is needed per bitness.
It is likely that Pep/10's effective bus size should be bumped to 32-bits, even if it can only access 16-bits of that space

### Insufficient Timing Accuracy
Another non-unified split exists between the MA2 and IS3 simulators.
The primary difference is that the ISA3 system is not currently capable of the sub-instruction cycle accuracy required to implement the MC2 bus protocol.
Ideally, the only difference should exists in the implementation of the CPU (tick recipient) and not the whole system.

### Simulator Pausing 
Pep9suite was able to "pause" the simulator mid-instruction.
This feature was used whenever memory-mapped input was requested but not buffered.
Focus was transfered to the interactive input until a character was received, and the memory access was completed.

Unfortunately, this was achieved by re-entering the event loop after adjusting some UI state (to prevent additional steps while paused).
WebAssembly does not support re-entering the Qt event loop.

While a minority of memory accesses trigger MMIO, the entire memory subsystem architecture essentially revolves around the fact memory accesses can always fail over the course of a useful program.
The current mechanism involves throwing on "missed" MMI and rolling-back the previous instruction using traces.
The net effect is that tracing must always be enabled when connected to a debugger.
It would be nice to loosen the tracing restriction.

### Virtual Memory Arenas for our Simulator
RISC-V can access 4GiB of address space, which must not be allocated eagerly.
With per-OS code, 64-bit platforms can use the host process's page table to enable lazy allocation plus copy-on-write semantics.
WebAssembly will have to perform some software fallback to enable lazy commit.

### Slow Simulator Start
There should be a fast path where an existing simulator can be copy-on-write forked.
When an operating system is first assembled a simulator will be created.
Whenever running a user program, the simulator with the preloaded OS will be forked rather than starting from scratch.
If I (ab)use my host process's page tables (see above point), I should be able to bring forking time into the double-digit microseconds.
Reducing start latency will reduce the latency incurred by hitting the "start debugging" button, which currently has a noticable  delay(~500ms).

## Decision
- Extend all addresses to 32-bits
- Implement virtual memory arenas to hardware-accelerate address decoding and copy-on-write simulators.
  32-bit platforms, like WASM, will need to fall back to a software-based lazy-commit.

## Constraints
- WebAssembly does not support mapping a 32-bit virtual address range inside the runtime.
- Avoid callback hell as an implementation detail in the simulator.
  Even if we introduce asynchronicity to the simulator (e.g., to let instructions pause), I want to write linear, imperative code for Pep/10.
  I understand that a timing-accurate RISC-V microarchitectural simulator will likely devolve to message-passing state machines, but Pep/10's processor should be simpler than that.

## Argument

In general, we usualy choose to sacrifice simulation speed in preference of accuracy and debugability.
For example:
- Pep9 had detailed per instruction timing statistics based on a microcoded CPU model
- Pepp logs exponential more bytes during instruction instruction tracing compared to [RISC-V e-trace](https://github.com/riscv-non-isa/riscv-trace-spec) or any other "real" instruction trace.
  Notably, our traces contain instruction+data traces compared to encoding only branch information.
  Those data traces are used to enable step-backwards, which is an invaluable debugging feature.
- Pepp trace output allow the effects of memory-mapped IO to be undone / rebuffered, which makes it possible to step back over IO instructions.
To that end, I'm willing to give up a bit of simulator performance to gain debugability

Some initial benchmarks to execute 100m instructions:
```
mmcraven@Polyvac output % hyperfine "./pepp-term mit" "./simexp imperative" "./simexp eventloop"
Benchmark 1: ./pepp-term mit
  Time (mean ± σ):      3.229 s ±  0.106 s    [User: 3.157 s, System: 0.034 s]
  Range (min … max):    3.161 s …  3.517 s    10 runs

Benchmark 2: ./simexp eventloop
  Time (mean ± σ):     761.8 ms ±   8.4 ms    [User: 749.9 ms, System: 9.1 ms]
  Range (min … max):   754.8 ms … 782.8 ms    10 runs

Benchmark 3: ./simexp imperative
  Time (mean ± σ):     288.9 ms ±   1.9 ms    [User: 281.7 ms, System: 4.6 ms]
  Range (min … max):   287.5 ms … 293.8 ms    10 runs

Summary
  ./simexp imperative ran
    2.64 ± 0.03 times faster than ./simexp eventloop
   11.18 ± 0.37 times faster than ./pepp-term mit
```

The `pepp-term mit` command executes 100m `here: br here` instructions using the existing imperative simulator.
that simulator uses virtual dispatch to select between the correct behavior for a specific core.
That virtual dispatch incurs an indirect / register-relative branch, must like the resumption of a coroutine.

Both `simexp` programs measure the overhead of different dispatch mechanisms without performing any actual instruction simulation.
`pc` is incremented, the result of Mem[PC] is declared to be djb(PC).
All instruction specifiers <0x80 are declared to be unary.
Non-unary instructions fetch (but do not decode!) the operand specifier.
There is no execute stage other than incrementing the instruction counter and arithmetic accumulation over is/os to prevent both branches from being optimized out.

The `imperative` variant coded in a similar style to the existing Pep simulator, except that it uses static dispatch rather than virtual.
`eventloop` posts messages to an event loop.
Each memory access is an event-loop post, as are synthetic delays inserted to preserve timing.
This version of eventloop attempts to bypass the eventloop when timing allows it.
When the eventloop is entered on each message, the time is 3x higher

In summary:
- An imperative simulator has a fixed overhead of ~3ms per million instructions.
- Based on `mit`, ~300ms per million instructions is required for the simulator control flow + memory reads + the execution portion.
  Of that 300ms, 297ms of that is the memread + execute, assuming the previous bullet point.
- `eventloop`-based dispatch overhead is ~7.5ms per million instructions when bypassing the event loop where possible.
  Without bypass, overhead is closer to 30ms per million instructiosn.
While not perfectly apples-to-applies, eventloop-based dispatch may inflate control-flow of the simulator overhead by a factor of 10.
But the vast majority of the simulation time is spent *doing* the instructions, not in control flow.

An eventloop simulator is trivial to make timing-accurate, which would unify the MA2 and ISA3 level simulators.
Since the eventloop is based on [coroutines](https://en.cppreference.com/cpp/language/coroutines), instructions become pausable.
