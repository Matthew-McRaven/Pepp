# Part 6: Relocations, Position Dependent Shared Libraries

## Relocations

As I said back in part 2, a relocation is a computation to perform on the contents. And as I said yesterday, a relocation can also direct the linker to take other actions, like creating a PLT or GOT entry. Let’s take a closer look at the computation.

In general a relocation has a type, a symbol, an offset into the contents, and an addend. From the linker’s point of view, the contents are simply an uninterpreted series of bytes. A relocation changes those bytes as necessary to produce the correct final executable. For example, consider the C code g = 0; where g is a global variable. On the i386, the compiler will turn this into an assembly language instruction, which will most likely be `movl $0, g` (for position dependent code–position independent code would loading the address of g from the GOT). Now, the g in the C code is a global variable, and we all more or less know what that means. The g in the assembly code is not that variable. It is a symbol which holds the address of that variable.

The assembler does not know the address of the global variable g, which is another way of saying that the assembler does not know the value of the symbol g. It is the linker that is going to pick that address. So the assembler has to tell the linker that it needs to use the address of g in this instruction. The way the assembler does this is to create a relocation. We don’t use a separate relocation type for each instruction; instead, each processor will have a natural set of relocation types which are appropriate for the machine architecture. Each type of relocation expresses a specific computation.

In the i386 case, the assembler will generate these bytes:

`c7 05 00 00 00 00 00 00 00 00`

The c7 05 are the instruction (movl constant to address). The first four 00 bytes are the 32-bit constant 0. The second four 00 bytes are the address. The assembler tells the linker to put the value of the symbol g into those four bytes by generating (in this case) a R\_386\_32 relocation. For this relocation the symbol will be g, the offset will be to the last four bytes of the instruction, the type will be R\_386\_32, and the addend will be 0 (in the case of the i386 the addend is stored in the contents rather than in the relocation itself, but this is a detail). The type R\_386\_32 expresses a specific computation, which is: put the 32-bit sum of the value of the symbol and the addend into the offset. Since for the i386 the addend is stored in the contents, this can also be expressed as: add the value of the symbol to the 32-bit field at the offset. When the linker performs this computation, the address in the instruction will be the address of the global variable g. Regardless of the details, the important point to note is that the relocation adjusts the contents by applying a specific computation selected by the type.

An example of a simple case which does use an addend would be

```
char a[10]; // A global array.
char* p = &a[1]; // In a function.
```

The assignment to p will wind up requiring a relocation for the symbol a. Here the addend will be 1, so that the resulting instruction references a + 1 rather than a + 0.

To point out how relocations are processor dependent, let’s consider g = 0; on a RISC processor: the PowerPC (in 32-bit mode). In this case, multiple assembly language instructions are required:

```
li 1,0 // Set register 1 to 0
lis 9,g@ha // Load high-adjusted part of g into register 9
stw 1,g@l(9) // Store register 1 to address in register 9 plus low adjusted part g
```

The lis instruction loads a value into the upper 16 bits of register 9, setting the lower 16 bits to zero. The stw instruction adds a signed 16 bit value to register 9 to form an address, and then stores the value of register 1 at that address. The @hapart of the operand directs the assembler to generate a R\_PPC\_ADDR16\_HA reloc. The @l produces a R\_PPC\_ADDR16\_LO reloc. The goal of these relocs is to compute the value of the symbol g and use it as the store address.

That is enough information to determine the computations performed by these relocs. The R\_PPC\_ADDR16\_HA reloc computes (SYMBOL >> 16) + ((SYMBOL & 0x8000) ? 1 : 0). The R\_PPC\_ADDR16\_LO computes SYMBOL & 0xffff. The extra computation for R\_PPC\_ADDR16\_HA is because the stw instruction adds the signed 16-bit value, which means that if the low 16 bits appears negative we have to adjust the high 16 bits accordingly. The offsets of the relocations are such that the 16-bit resulting values are stored into the appropriate parts of the machine instructions.

The specific examples of relocations I’ve discussed here are ELF specific, but the same sorts of relocations occur for any object file format.

The examples I’ve shown are for relocations which appear in an object file. As discussed in part 4, these types of relocations may also appear in a shared library, if they are copied there by the program linker. In ELF, there are also specific relocation types which never appear in object files but only appear in shared libraries or executables. These are the JMP\_SLOT, GLOB\_DAT, and RELATIVE relocations discussed earlier. Another type of relocation which only appears in an executable is a COPY relocation, which I will discuss later.

## Position Dependent Shared Libraries

I realized that in part 4 I forgot to say one of the important reasons that ELF shared libraries use PLT and GOT tables. The idea of a shared library is to permit mapping the same shared library into different processes. This only works at maximum efficiency if the shared library code looks the same in each process. If it does not look the same, then each process will need its own private copy, and the savings in physical memory and sharing will be lost.

As discussed in part 4, when the dynamic linker loads a shared library which contains position dependent code, it must apply a set of dynamic relocations. Those relocations will change the code in the shared library, and it will no longer be sharable.

The advantage of the PLT and GOT is that they move the relocations elsewhere, to the PLT and GOT tables themselves. Those tables can then be put into a read-write part of the shared library. This part of the shared library will be much smaller than the code. The PLT and GOT tables will be different in each process using the shared library, but the code will be the same.

I’ll be taking a vacation for the long weekend. My next post will most likely be on Tuesday.
