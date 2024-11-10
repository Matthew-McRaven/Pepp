# Part 13: Symbol resolution from the user's point of view; Static Linking vs. Dynamic Linking

## Symbol resolution from the user's point of view

I’ve talked about symbol versions from the linker’s point of view. I think it’s worth discussing them a bit from the user’s point of view.

As I’ve discussed before, symbol versions are an ELF extension designed to solve a specific problem: making it possible to upgrade a shared library without changing existing executables. That is, they provide backward compatibility for shared libraries. There are a number of related problems which symbol versions do not solve. They do not provide forward compatibility for shared libraries: if you upgrade your executable, you may need to upgrade your shared library also (it would be nice to have a feature to build your executable against an older version of the shared library, but that is difficult to implement in practice). They only work at the shared library interface: they do not help with a change to the ABI of a system call, which is at the kernel interface. They do not help with the problem of sharing incompatible versions of a shared library, as may happen when a complex application is built out of several different existing shared libraries which have incompatible dependencies.

Despite these limitations, shared library backward compatibility is an important issue. Using symbol versions to ensure backward compatibility requires a careful and rigorous approach. You must start by applying a version to every symbol. If a symbol in the shared library does not have a version, then it is impossible to change it in a backward compatible fashion. Then you must pay close attention to the ABI of every symbol. If the ABI of a symbol changes for any reason, you must provide a copy which implements the old ABI. That copy should be marked with the original version. The new symbol must be given a new version.

The ABI of a symbol can change in a number of ways. Any change to the parameter types or the return type of a function is an ABI change. Any change in the type of a variable is an ABI change. If a parameter or a return type is a struct or class, then any change in the type of any field is an ABI change–i.e., if a field in a struct points to another struct, and that struct changes, the ABI has changed. If a function is defined to return an instance of an enum, and a new value is added to the enum, that is an ABI change. In other words, even minor changes can be ABI changes. The question you need to ask is: can existing code which has already been compiled continue to use the new symbol with no change? If the answer is no, you have an ABI change, and you must define a new symbol version.

You must be very careful when writing the symbol implementing the old ABI, if you don’t just copy the existing code. You must be certain that it really does implement the old ABI.

There are some special challenges when using C++. Adding a new virtual method to a class can be an ABI change for any function which uses that class. Providing the backward compatible version of the class in such a situation is very awkward–there is no natural way to specify the name and version to use for the virtual table or the RTTI information for the old version.

Naturally, you must never delete any symbols.

Getting all the details correct, and verifying that you got them correct, requires great attention to detail. Unfortunately, I don’t know of any tools to help people write correct version scripts, or to verify them. Still, if implemented correctly, the results are good: existing executables will continue to run.

## Static Linking vs. Dynamic Linking

There is, of course, another way to ensure that existing executables will continue to run: link them statically, without using any shared libraries. That will limit their ABI issues to the kernel interface, which is normally significantly smaller than the library interface.

There is a performance tradeoff with static linking. A statically linked program does not get the benefit of sharing libraries with other programs executing at the same time. On the other hand, a statically linked program does not have to pay the performance penalty of position independent code when executing within the library.

Upgrading the shared library is only possible with dynamic linking. Such an upgrade can provide bug fixes and better performance. Also, the dynamic linker can select a version of the shared library appropriate for the specific platform, which can also help performance.

Static linking permits more reliable testing of the program. You only need to worry about kernel changes, not about shared library changes.

Some people argue that dynamic linking is always superior. I think there are benefits on both sides, and which choice is best depends on the specific circumstances.

More on Monday. If you think I should write about any specific linker related topics which have not already been mentioned in the comments, please let me know.
