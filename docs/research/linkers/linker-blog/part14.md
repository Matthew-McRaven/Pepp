# Part 14: Link time optimization, aka Whole Program optimization; Initialization Code

## Link Time Optimization

I’ve already mentioned some optimizations which are peculiar to the linker: relaxation and garbage collection of unwanted sections. There is another class of optimizations which occur at link time, but are really related to the compiler. The general name for these optimizations is link time optimization or whole program optimization.

The general idea is that the compiler optimization passes are run at link time. The advantage of running them at link time is that the compiler can then see the entire program. This permits the compiler to perform optimizations which can not be done when sources files are compiled separately. The most obvious such optimization is inlining functions across source files. Another is optimizing the calling sequence for simple functions–e.g., passing more parameters in registers, or knowing that the function will not clobber all registers; this can only be done when the compiler can see all callers of the function. Experience shows that these and other optimizations can bring significant performance benefits.

Generally these optimizations are implemented by having the compiler write a version of its intermediate representation into the object file, or into some parallel file. The intermediate representation will be the parsed version of the source file, and may already have had some local optimizations applied. Sometimes the object file contains only the compiler intermediate representation, sometimes it also contains the usual object code. In the former case link time optimization is required, in the latter case it is optional.

I know of two typical ways to implement link time optimization. The first approach is for the compiler to provide a pre-linker. The pre-linker examines the object files looking for stored intermediate representation. When it finds some, it runs the link time optimization passes. The second approach is for the linker proper to call back into the compiler when it finds intermediate representation. This is generally done via some sort of plugin API.

Although these optimizations happen at link time, they are not part of the linker proper, at least not as I defined it. When the compiler reads the stored intermediate representation, it will eventually generate an object file, one way or another. The linker proper will then process that object file as usual. These optimizations should be thought of as part of the compiler.

## Initialization Code

C++ permits globals variables to have constructors and destructors. The global constructors must be run before main starts, and the global destructors must be run after exit is called. Making this work requires the compiler and the linker to cooperate.

The a.out object file format is rarely used these days, but the GNU a.out linker has an interesting extension. In a.out symbols have a one byte type field. This encodes a bunch of debugging information, and also the section in which the symbol is defined. The a.out object file format only supports three sections–text, data, and bss. Four symbol types are defined as sets: text set, data set, bss set, and absolute set. A symbol with a set type is permitted to be defined multiple times. The GNU linker will not give a multiple definition error, but will instead build a table with all the values of the symbol. The table will start with one word holding the number of entries, and will end with a zero word. In the output file the set symbol will be defined as the address of the start of the table.

For each C++ global constructor, the compiler would generate a symbol named **CTOR\_LIST** with the text set type. The value of the symbol in the object file would be the global constructor function. The linker would gather together all the **CTOR\_LIST** functions into a table. The startup code supplied by the compiler would walk down the **CTOR\_LIST** table and call each function. Global destructors were handled similarly, with the name **DTOR\_LIST**.

Anyhow, so much for a.out. In ELF, global constructors are handled in a fairly similar way, but without using magic symbol types. I’ll describe what gcc does. An object file which defines a global constructor will include a .ctors section. The compiler will arrange to link special object files at the very start and very end of the link. The one at the start of the link will define a symbol for the .ctors section; that symbol will wind up at the start of the section. The one at the end of the link will define a symbol for the end of the .ctors section. The compiler startup code will walk between the two symbols, calling the constructors. Global destructors work similarly, in a .dtors section.

ELF shared libraries work similarly. When the dynamic linker loads a shared library, it will call the function at the DT\_INIT tag if there is one. By convention the ELF program linker will set this to the function named \_init, if there is one. Similarly the DT\_FINI tag is called when a shared library is unloaded, and the program linker will set this to the function named \_fini.

As I mentioned earlier, three are also DT\_INIT\_ARRAY, DT\_PREINIT\_ARRAY, and DT\_FINI\_ARRAY tags, which are set based on the SHT\_INIT\_ARRAY, SHT\_PREINIT\_ARRAY, and SHT\_FINI\_ARRAY section types. This is a newer approach in ELF, and does not require relying on special symbol names.

More tomorrow.
