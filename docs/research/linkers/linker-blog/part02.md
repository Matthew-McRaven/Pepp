# Part 2: What's-a-linker: Dynamic linking, linker data types, linker operation

## What's a Linker?

I’m back, and I’m still doing the linker technical introduction.

Shared libraries were invented as an optimization for virtual memory systems running many processes simultaneously. People noticed that there is a set of basic functions which appear in almost every program. Before shared libraries, in a system which runs multiple processes simultaneously, that meant that almost every process had a copy of exactly the same code. This suggested that on a virtual memory system it would be possible to arrange that code so that a single copy could be shared by every process using it. The virtual memory system would be used to map the single copy into the address space of each process which needed it. This would require less physical memory to run multiple programs, and thus yield better performance.

I believe the first implementation of shared libraries was on SVR3, based on COFF. This implementation was simple, and basically assigned each shared library a fixed portion of the virtual address space. This did not require any significant changes to the linker. However, requiring each shared library to reserve an appropriate portion of the virtual address space was inconvenient.

SunOS4 introduced a more flexible version of shared libraries, which was later picked up by SVR4. This implementation postponed some of the operation of the linker to runtime. When the program started, it would automatically run a limited version of the linker which would link the program proper with the shared libraries. The version of the linker which runs when the program starts is known as the dynamic linker. When it is necessary to distinguish them, I will refer to the version of the linker which creates the program as the program linker. This type of shared libraries was a significant change to the traditional program linker: it now had to build linking information which could be used efficiently at runtime by the dynamic linker.

That is the end of the introduction. You should now understand the basics of what a linker does. I will now turn to how it does it.

## Basic Linker Data Types

The linker operates on a small number of basic data types: symbols, relocations, and contents. These are defined in the input object files. Here is an overview of each of these.

A symbol is basically a name and a value. Many symbols represent static objects in the original source code–that is, objects which exist in a single place for the duration of the program. For example, in an object file generated from C code, there will be a symbol for each function and for each global and static variable. The value of such a symbol is simply an offset into the contents. This type of symbol is known as a defined symbol. It’s important not to confuse the value of the symbol representing the variable my\_global\_var with the value of my\_global\_var itself. The value of the symbol is roughly the address of the variable: the value you would get from the expression \&my\_global\_var in C.

Symbols are also used to indicate a reference to a name defined in a different object file. Such a reference is known as an undefined symbol. There are other less commonly used types of symbols which I will describe later.

During the linking process, the linker will assign an address to each defined symbol, and will resolve each undefined symbol by finding a defined symbol with the same name.

A relocation is a computation to perform on the contents. Most relocations refer to a symbol and to an offset within the contents. Many relocations will also provide an additional operand, known as the addend. A simple, and commonly used, relocation is “set this location in the contents to the value of this symbol plus this addend.” The types of computations that relocations do are inherently dependent on the architecture of the processor for which the linker is generating code. For example, RISC processors which require two or more instructions to form a memory address will have separate relocations to be used with each of those instructions; for example, “set this location in the contents to the lower 16 bits of the value of this symbol.”

During the linking process, the linker will perform all of the relocation computations as directed. A relocation in an object file may refer to an undefined symbol. If the linker is unable to resolve that symbol, it will normally issue an error (but not always: for some symbol types or some relocation types an error may not be appropriate).

The contents are what memory should look like during the execution of the program. Contents have a size, an array of bytes, and a type. They contain the machine code generated by the compiler and assembler (known as text). They contain the values of initialized variables (data). They contain static unnamed data like string constants and switch tables (read-only data or rdata). They contain uninitialized variables, in which case the array of bytes is generally omitted and assumed to contain only zeroes (bss). The compiler and the assembler work hard to generate exactly the right contents, but the linker really doesn’t care about them except as raw data. The linker reads the contents from each file, concatenates them all together sorted by type, applies the relocations, and writes the result into the executable file.

## Basic Linker Operation

At this point we already know enough to understand the basic steps used by every linker.

* Read the input object files. Determine the length and type of the contents. Read the symbols.
* Build a symbol table containing all the symbols, linking undefined symbols to their definitions.
* Decide where all the contents should go in the output executable file, which means deciding where they should go in memory when the program runs.
* Read the contents data and the relocations. Apply the relocations to the contents. Write the result to the output file.
* Optionally write out the complete symbol table with the final values of the symbols.
