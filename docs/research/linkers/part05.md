# Part 5: More Shared Libraries -- specifically, linker implementation; ELF Symbols

## Shared Libraries

Yesterday I talked about how shared libraries work. I realized that I should say something about how linkers implement shared libraries. This discussion will again be ELF specific.

When the program linker puts position dependent code into a shared library, it has to copy more of the relocations from the object file into the shared library. They will become dynamic relocations computed by the dynamic linker at runtime. Some relocations do not have to be copied; for example, a PC relative relocation to a symbol which is local to shared library can be fully resolved by the program linker, and does not require a dynamic reloc. However, note that a PC relative relocation to a global symbol does require a dynamic relocation; otherwise, the main executable would not be able to override the symbol. Some relocations have to exist in the shared library, but do not need to be actual copies of the relocations in the object file; for example, a relocation which computes the absolute address of symbol which is local to the shared library can often be replaced with a RELATIVE reloc, which simply directs the dynamic linker to add the difference between the shared library’s load address and its base address. The advantage of using a RELATIVE reloc is that the dynamic linker can compute it quickly at runtime, because it does not require determining the value of a symbol.

For position independent code, the program linker has a harder job. The compiler and assembler will cooperate to generate spcial relocs for position independent code. Although details differ among processors, there will typically be a PLT reloc and a GOT reloc. These relocs will direct the program linker to add an entry to the PLT or the GOT, as well as performing some computation. For example, on the i386 a function call in position independent code will generate a R\_386\_PLT32 reloc. This reloc will refer to a symbol as usual. It will direct the program linker to add a PLT entry for that symbol, if one does not already exist. The computation of the reloc is then a PC-relative reference to the PLT entry. (The 32 in the name of the reloc refers to the size of the reference, which is 32 bits). Yesterday I described how on the i386 every PLT entry also has a corresponding GOT entry, so the R\_386\_PLT32 reloc actually directs the program linker to create both a PLT entry and a GOT entry.

When the program linker creates an entry in the PLT or the GOT, it must also generate a dynamic reloc to tell the dynamic linker about the entry. This will typically be a JMP\_SLOT or GLOB\_DAT relocation.

This all means that the program linker must keep track of the PLT entry and the GOT entry for each symbol. Initially, of course, there will be no such entries. When the linker sees a PLT or GOT reloc, it must check whether the symbol referenced by the reloc already has a PLT or GOT entry, and create one if it does not. Note that it is possible for a single symbol to have both a PLT entry and a GOT entry; this will happen for position independent code which both calls a function and also takes its address.

The dynamic linker’s job for the PLT and GOT tables is to simply compute the JMP\_SLOT and GLOB\_DAT relocs at runtime. The main complexity here is the lazy evaluation of PLT entries which I described yesterday.

The fact that C permits taking the address of a function introduces an interesting wrinkle. In C you are permitted to take the address of a function, and you are permitted to compare that address to another function address. The problem is that if you take the address of a function in a shared library, the natural result would be to get the address of the PLT entry. After all, that is address to which a call to the function will jump. However, each shared library has its own PLT, and thus the address of a particular function would differ in each shared library. That means that comparisons of function pointers generated in different shraed libraries may be different when they should be the same. This is not a purely hypothetical problem; when I did a port which got it wrong, before I fixed the bug I saw failures in the Tcl shared library when it compared function pointers.

The fix for this bug on most processors is a special marking for a symbol which has a PLT entry but is not defined. Typically the symbol will be marked as undefined, but with a non-zero value–the value will be set to the address of the PLT entry. When the dynamic linker is searching for the value of a symbol to use for a reloc other than a JMP\_SLOT reloc, if it finds such a specially marked symbol, it will use the non-zero value. This will ensure that all references to the symbol which are not function calls will use the same value. To make this work, the compiler and assembler must make sure that any reference to a function which does not involve calling it will not carry a standard PLT reloc. This special handling of function addresses needs to be implemented in both the program linker and the dynamic linker.

## ELF Symbols

OK, enough about shared libraries. Let’s go over ELF symbols in more detail. I’m not going to lay out the exact data structures–go to the ELF ABI for that. I’m going to take about the different fields and what they mean. Many of the different types of ELF symbols are also used by other object file formats, but I won’t cover that.

An entry in an ELF symbol table has eight pieces of information: a name, a value, a size, a section, a binding, a type, a visibility, and undefined additional information (currently there are six undefined bits, though more may be added). An ELF symbol defined in a shared object may also have an associated version name.

The name is obvious.

For an ordinary defined symbol, the section is some section in the file (specifically, the symbol table entry holds an index into the section table). For an object file the value is relative to the start of the section. For an executable the value is an absolute address. For a shared library the value is relative to the base address.

For an undefined reference symbol, the section index is the special value SHN\_UNDEF which has the value 0. A section index of SHN\_ABS (0xfff1) indicates that the value of the symbol is an absolute value, not relative to any section.

A section index of SHN\_COMMON (0xfff2) indicates a common symbol. Common symbols were invented to handle Fortran common blocks, and they are also often used for uninitialized global variables in C. A common symbol has unusual semantics. Common symbols have a value of zero, but set the size field to the desired size. If one object file has a common symbol and another has a definition, the common symbol is treated as an undefined reference. If there is no definition for a common symbol, the program linker acts as though it saw a definition initialized to zero of the appropriate size. Two object files may have common symbols of different sizes, in which case the program linker will use the largest size. Implementing common symbol semantics across shared libraries is a touchy subject, somewhat helped by the recent introduction of a type for common symbols as well as a special section index (see the discussion of symbol types below).

The size of an ELF symbol, other than a common symbol, is the size of the variable or function. This is mainly used for debugging purposes.

The binding of an elf symbol is global, local, or weak. A global symbol is globally visible. A local symbol is only locally visible (e.g., a static function). Weak symbols come in two flavors. A weak undefined reference is like an ordinary undefined reference, except that it is not an error if a relocation refers to a weak undefined reference symbol which has no defining symbol. Instead, the relocation is computed as though the symbol had the value zero.

A weak defined symbol is permitted to be linked with a non-weak defined symbol of the same name without causing a multiple definition error. Historically there are two ways for the program linker to handle a weak defined symbol. On SVR4 if the program linker sees a weak defined symbol followed by a non-weak defined symbol with the same name, it will issue a multiple definition error. However, a non-weak defined symbol followed by a weak defined symbol will not cause an error. On Solaris, a weak defined symbol followed by a non-weak defined symbol is handled by causing all references to attach to the non-weak defined symbol, with no error. This difference in behaviour is due to an ambiguity in the ELF ABI which was read differently by different people. The GNU linker follows the Solaris behaviour.

The type of an ELF symbol is one of the following:

* STT\_NOTYPE: no particular type.
* STT\_OBJECT: a data object, such as a variable.
* STT\_FUNC: a function
* STT\_SECTION: a local symbol associated with a section. This type of symbol is used to reduce the number of local symbols required, by changing all relocations against local symbols in a specific section to use the STT\_SECTION symbol instead.
* STT\_FILE: a special symbol whose name is the name of the source file which produced the object file.
* STT\_COMMON: a common symbol. This is the same as setting the section index to SHN\_COMMON, except in a shared object. The program linker will normally have allocated space for the common symbol in the shared object, so it will have a real section index. The STT\_COMMON type tells the dynamic linker that although the symbol has a regular definition, it is a common symbol.
* STT\_TLS: a symbol in the Thread Local Storage area. I will describe this in more detail some other day.

ELF symbol visibility was invented to provide more control over which symbols were accessible outside a shared library. The basic idea is that a symbol may be global within a shared library, but local outside the shared library.

* STV\_DEFAULT: the usual visibility rules apply: global symbols are visible everywhere.
* STV\_INTERNAL: the symbol is not accessible outside the current executable or shared library.
* STV\_HIDDEN: the symbol is not visible outside the current executable or shared library, but it may be accessed indirectly, probably because some code took its address.
* STV\_PROTECTED: the symbol is visible outside the current executable or shared object, but it may not be overridden. That is, if a protected symbol in a shared library is referenced by other code in the shared library, that other code will always reference the symbol in the shared library, even if the executable defines a symbol with the same name. I’ll described symbol versions later.

More tomorrow.
