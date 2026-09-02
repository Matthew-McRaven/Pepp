link.ld and start.S are meant to fake a Linux environment.
I want statically linked executables to reduce build system dependencies, so there is no RISCV ldd.

While a full system emulator could use ldd and patch the exectuable, I don't want to provide that library in CI / local builds.
Instead, I set up a stack, and ensure that the symbol name _start appears in the program.

For test cases which assume a portion of the stdlib exist, I use some thin C-to-ASM wrappers.
