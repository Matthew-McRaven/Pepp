# Part 4: Shared Libraries

We’ve talked a bit about what object files and executables look like, so what do shared libraries look like? I’m going to focus on ELF shared libraries as used in SVR4 (and GNU/Linux, etc.), as they are the most flexible shared library implementation and the one I know best.

Windows shared libraries, known as DLLs, are less flexible in that you have to compile code differently depending on whether it will go into a shared library or not. You also have to express symbol visibility in the source code. This is not inherently bad, and indeed ELF has picked up some of these ideas over time, but the ELF format makes more decisions at link time and is thus more powerful.

When the program linker creates a shared library, it does not yet know which virtual address that shared library will run at. In fact, in different processes, the same shared library will run at different address, depending on the decisions made by the dynamic linker. This means that shared library code must be position independent. More precisely, it must be position independent after the dynamic linker has finished loading it. It is always possible for the dynamic linker to convert any piece of code to run at any virtula address, given sufficient relocation information. However, performing the reloc computations must be done every time the program starts, implying that it will start more slowly. Therefore, any shared library system seeks to generate position independent code which requires a minimal number of relocations to be applied at runtime, while still running at close to the runtime efficiency of position dependent code.

An additional complexity is that ELF shared libraries were designed to be roughly equivalent to ordinary archives. This means that by default the main executable may override symbols in the shared library, such that references in the shared library will call the definition in the executable, even if the shared library also defines that same symbol. For example, an executable may define its own version of malloc. The C library also defines malloc, and the C library contains code which calls malloc. If the executable defines malloc itself, it will override the function in the C library. When some other function in the C library calls malloc, it will call the definition in the executable, not the definition in the C library.

There are thus different requirements pulling in different directions for any specific ELF implementation. The right implementation choices will depend on the characteristics of the processor. That said, most, but not all, processors make fairly similar decisions. I will describe the common case here. An example of a processor which uses the common case is the i386; an example of a processor which make some different decisions is the PowerPC.

In the common case, code may be compiled in two different modes. By default, code is position dependent. Putting position dependent code into a shared library will cause the program linker to generate a lot of relocation information, and cause the dynamic linker to do a lot of processing at runtime. Code may also be compiled in position independent mode, typically with the -fpic option. Position independent code is slightly slower when it calls a non-static function or refers to a global or static variable. However, it requires much less relocation information, and thus the dynamic linker will start the program faster.

Position independent code will call non-static functions via the Procedure Linkage Table or PLT. This PLT does not exist in .o files. In a .o file, use of the PLT is indicated by a special relocation. When the program linker processes such a relocation, it will create an entry in the PLT. It will adjust the instruction such that it becomes a PC-relative call to the PLT entry. PC-relative calls are inherently position independent and thus do not require a relocation entry themselves. The program linker will create a relocation for the PLT entry which tells the dynamic linker which symbol is associated with that entry. This process reduces the number of dynamic relocations in the shared library from one per function call to one per function called.

Further, PLT entries are normally relocated lazily by the dynamic linker. On most ELF systems this laziness may be overridden by setting the LD\_BIND\_NOW environment variable when running the program. However, by default, the dynamic linker will not actually apply a relocation to the PLT until some code actually calls the function in question. This also speeds up startup time, in that many invocations of a program will not call every possible function. This is particularly true when considering the shared C library, which has many more function calls than any typical program will execute.

In order to make this work, the program linker initializes the PLT entries to load an index into some register or push it on the stack, and then to branch to common code. The common code calls back into the dynamic linker, which uses the index to find the appropriate PLT relocation, and uses that to find the function being called. The dynamic linker then initializes the PLT entry with the address of the function, and then jumps to the code of the function. The next time the function is called, the PLT entry will branch directly to the function.

Before giving an example, I will talk about the other major data structure in position independent code, the Global Offset Table or GOT. This is used for global and static variables. For every reference to a global variable from position independent code, the compiler will generate a load from the GOT to get the address of the variable, followed by a second load to get the actual value of the variable. The address of the GOT will normally be held in a register, permitting efficient access. Like the PLT, the GOT does not exist in a .o file, but is created by the program linker. The program linker will create the dynamic relocations which the dynamic linker will use to initialize the GOT at runtime. Unlike the PLT, the dynamic linker always fully initializes the GOT when the program starts.

For example, on the i386, the address of the GOT is held in the register %ebx. This register is initialized at the entry to each function in position independent code. The initialization sequence varies from one compiler to another, but typically looks something like this:

```
call __i686.get_pc_thunk.bx
add $offset,%ebx
```

The function `__i686.get_pc_thunk.bx` simply looks like this:

```
mov (%esp),%ebx
ret
```

This sequence of instructions uses a position independent sequence to get the address at which it is running. Then is uses an offset to get the address of the GOT. Note that this requires that the GOT always be a fixed offset from the code, regardless of where the shared library is loaded. That is, the dynamic linker must load the shared library as a fixed unit; it may not load different parts at varying addresses.

Global and static variables are now read or written by first loading the address via a fixed offset from %ebx. The program linker will create dynamic relocations for each entry in the GOT, telling the dynamic linker how to initialize the entry. These relocations are of type GLOB\_DAT.

For function calls, the program linker will set up a PLT entry to look like this:

```
jmp *offset(%ebx)
pushl #index
jmp first_plt_entry
```

The program linker will allocate an entry in the GOT for each entry in the PLT. It will create a dynamic relocation for the GOT entry of type JMP\_SLOT. It will initialize the GOT entry to the base address of the shared library plus the address of the second instruction in the code sequence above. When the dynamic linker does the initial lazy binding on a JMP\_SLOT reloc, it will simply add the difference between the shared library load address and the shared library base address to the GOT entry. The effect is that the first jmp instruction will jump to the second instruction, which will push the index entry and branch to the first PLT entry. The first PLT entry is special, and looks like this:

```
pushl 4(%ebx)
jmp *8(%ebx)
```

This references the second and third entries in the GOT. The dynamic linker will initialize them to have appropriate values for a callback into the dynamic linker itself. The dynamic linker will use the index pushed by the first code sequence to find the JMP\_SLOT relocation. When the dynamic linker determines the function to be called, it will store the address of the function into the GOT entry references by the first code sequence. Thus, the next time the function is called, the jmp instruction will branch directly to the right code.

That was a fast pass over a lot of details, but I hope that it conveys the main idea. It means that for position independent code on the i386, every call to a global function requires one extra instruction after the first time it is called. Every reference to a global or static variable requires one extra instruction. Almost every function uses four extra instructions when it starts to initialize %ebx (leaf functions which do not refer to any global variables do not need to initialize %ebx). This all has some negative impact on the program cache. This is the runtime performance penalty paid to let the dynamic linker start the program quickly.

On other processors, the details are naturally different. However, the general flavour is similar: position independent code in a shared library starts faster and runs slightly slower.

More tomorrow.
