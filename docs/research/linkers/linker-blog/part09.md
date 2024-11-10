# Part 9: Symbol Versions, Relaxation optimization

## Symbol Versions

A shared library provides an API. Since executables are built with a specific set of header files and linked against a specific instance of the shared library, it also provides an ABI. It is desirable to be able to update the shared library independently of the executable. This permits fixing bugs in the shared library, and it also permits the shared library and the executable to be distributed separately. Sometimes an update to the shared library requires changing the API, and sometimes changing the API requires changing the ABI. When the ABI of a shared library changes, it is no longer possible to update the shared library without updating the executable. This is unfortunate.

For example, consider the system C library and the stat function. When file systems were upgraded to support 64-bit file offsets, it became necessary to change the type of some of the fields in the stat struct. This is a change in the ABI of stat. New versions of the system library should provide a stat which returns 64-bit values. But old existing executables call stat expecting 32-bit values. This could be addressed by using complicated macros in the system header files. But there is a better way.

The better way is symbol versions, which were introduced at Sun and extended by the GNU tools. Every shared library may define a set of symbol versions, and assign specific versions to each defined symbol. The versions and symbol assignments are done by a script passed to the program linker when creating the shared library.

When an executable or shared library A is linked against another shared library B, and A refers to a symbol S defined in B with a specific version, the undefined dynamic symbol reference S in A is given the version of the symbol S in B. When the dynamic linker sees that A refers to a specific version of S, it will link it to that specific version in B. If B later introduces a new version of S, this will not affect A, as long as B continues to provide the old version of S.

For example, when stat changes, the C library would provide two versions of stat, one with the old version (e.g., LIBC\_1.0), and one with the new version (LIBC\_2.0). The new version of stat would be marked as the default–the program linker would use it to satisfy references to stat in object files. Executables linked against the old version would require the LIBC\_1.0 version of stat, and would therefore continue to work. Note that it is even possible for both versions of stat to be used in a single program, accessed from different shared libraries.

As you can see, the version effectively is part of the name of the symbol. The biggest difference is that a shared library can define a specific version which is used to satisfy an unversioned reference.

Versions can also be used in an object file (this is a GNU extension to the original Sun implementation). This is useful for specifying versions without requiring a version script. When a symbol name containts the @ character, the string before the @ is the name of the symbol, and the string after the @ is the version. If there are two consecutive @ characters, then this is the default version.

## Relaxation

Generally the program linker does not change the contents other than applying relocations. However, there are some optimizations which the program linker can perform at link time. One of them is relaxation.

Relaxation is inherently processor specific. It consists of optimizing code sequences which can become smaller or more efficient when final addresses are known. The most common type of relaxation is for call instructions. A processor like the m68k supports different PC relative call instructions: one with a 16-bit offset, and one with a 32-bit offset. When calling a function which is within range of the 16-bit offset, it is more efficient to use the shorter instruction. The optimization of shrinking these instructions at link time is known as relaxation.

Relaxation is applied based on relocation entries. The linker looks for relocations which may be relaxed, and checks whether they are in range. If they are, the linker applies the relaxation, probably shrinking the size of the contents. The relaxation can normally only be done when the linker recognizes the instruction being relocated. Applying a relaxation may in turn bring other relocations within range, so relaxation is typically done in a loop until there are no more opportunities.

When the linker relaxes a relocation in the middle of a contents, it may need to adjust any PC relative references which cross the point of the relaxation. Therefore, the assembler needs to generate relocation entries for all PC relative references. When not relaxing, these relocations may not be required, as a PC relative reference within a single contents will be valid whereever the contents winds up. When relaxing, though, the linker needs to look through all the other relocations that apply to the contents, and adjust PC relatives one where appropriate. This adjustment will simply consist of recomputing the PC relative offset.

Of course it is also possible to apply relaxations which do not change the size of the contents. For example, on the MIPS the position independent calling sequence is normally to load the address of the function into the $25 register and then to do an indirect call through the register. When the target of the call is within the 18-bit range of the branch-and-call instruction, it is normally more efficient to use branch-and-call, since then the processor does not have to wait for the load of $25 to complete before starting the call. This relaxation changes the instruction sequence without changing the size.

More tomorrow. I apologize for the haphazard arrangement of these linker notes. I’m just writing about ideas as I think of them, rather than being organized about that. If I do collect these notes into an essay, I’ll try to make them more structured.
