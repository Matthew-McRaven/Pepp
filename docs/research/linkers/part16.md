# Part 16: C++ Template Instantiation, Exception Frames

## C++ Template Instantiation

There is still more C++ fun at link time, though somewhat less related to the linker proper. A C++ program can declare templates, and instantiate them with specific types. Ideally those specific instantiations will only appear once in a program, not once per source file which instantiates the templates. There are a few ways to make this work.

For object file formats which support COMDAT and vague linkage, which I described yesterday, the simplest and most reliable mechanism is for the compiler to generate all the template instantiations required for a source file and put them into the object file. They should be marked as COMDAT, so that the linker discards all but one copy. This ensures that all template instantiations will be available at link time, and that the executable will have only one copy. This is what gcc does by default for systems which support it. The obvious disadvantages are the time required to compile all the duplicate template instantiations and the space they take up in the object files. This is sometimes called the Borland model, as this is what Borland’s C++ compiler did.

Another approach is to not generate any of the template instantiations at compile time. Instead, when linking, if we need a template instantiation which is not found, invoke the compiler to build it. This can be done either by running the linker and looking for error messages or by using a linker plugin to handle an undefined symbol error. The difficulties with this approach are to find the source code to compile and to find the right options to pass to the compiler. Typically the source code is placed into a repository file of some sort at compile time, so that it is available at link time. The complexities of getting the compilation steps right are why this approach is not the default. When it works, though, it can be faster than the duplicate instantiation approach. This is sometimes called the Cfront model.

gcc also supports explicit template instantiation, which can be used to control exactly where templates are instantiated. This approach can work if you have complete control over your source code base, and can instantiate all required templates in some central place. This approach is used for gcc’s C++ library, libstdc++.

C++ defines a keyword export which is supposed to permit exporting template definitions in such a way that they can be read back in by the compiler. gcc does not support this keyword. If it worked, it could be a slightly more reliable way of using a repository when using the Cfront model.

## Exception Frames

C++ and other languages support exceptions. When an exception is thrown in one function and caught in another, the program needs to reset the stack pointer and registers to the point where the exception is caught. While resetting the stack pointer, the program needs to identify all local variables in the part of the stack being discarded, and run their destructors if any. This process is known as unwinding the stack.

The information needed to unwind the stack is normally stored in tables in the program. Supporting library code is used to read the tables and perform the necessary operations. I’m not going to describe the details of those tables here. However, there is a linker optimization which applies to them.

The support libraries need to be able to find the exception tables at runtime when an exception occurs. An exception can be thrown in one shared library and caught in a different shared library, so finding all the required exception tables can be a nontrivial operation. One approach that can be used is to register the exception tables at program startup time or shared library load time. The registration can be done at the right time using the global constructor mechanism.

However, this approach imposes a runtime cost for exceptions, in that it takes longer for the program to start. Therefore, this is not ideal. The linker can optimize this by building tables which can be used to find the exception tables. The tables built by the GNU linker are sorted for fast lookup by the runtime library. The tables are put into a PT\_GNU\_EH\_FRAME segment. The supporting libraries then need a way to look up a segment of this type. This is done via the dl\_iterate\_phdr API provided by the GNU dynamic linker.

Note that if the compiler believes that the linker will generate a PT\_GNU\_EH\_FRAME segment, it won’t generate the startup code to register the exception tables. Thus the linker must not fail to create this segment.

Since the GNU linker needs to look at the exception tables in order to generate the PT\_GNU\_EH\_FRAME segment, it will also optimize by discarding duplicate exception table information.

I know this is section is rather short on details. I hope the general idea is clear.

More tomorrow.
