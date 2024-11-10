# Part 12: Symbol Resolution

I apologize for the pause in posts. We moved over the weekend. Last Friday at\&t told me that the new DSL was working at our new house. However, it did not actually start working outside the house until Wednesday. Then a problem with the internal wiring meant that it was not working inside the house until today. I am now finally back online at home.

Symbol Resolution

I find that symbol resolution is one of the trickier aspects of a linker. Symbol resolution is what the linker does the second and subsequent times that it sees a particular symbol. I’ve already touched on the topic in a few previous entries, but let’s look at it in a bit more depth.

Some symbols are local to a specific object files. We can ignore these for the purposes of symbol resolution, as by definition the linker will never see them more than once. In ELF these are the symbols with a binding of STB\_LOCAL.

In general, symbols are resolved by name: every symbol with the same name is the same entity. We’ve already seen a few exceptions to that general rule. A symbol can have a version: two symbols with the same name but different versions are different symbols. A symbol can have non-default visibility: a symbol with hidden visibility in one shared library is not the same as a symbol with the same name in a different shared library.

The characteristics of a symbol which matter for resolution are:

* The symbol name
* The symbol version.
* Whether the symbol is the default version or not.
* Whether the symbol is a definition or a reference or a common symbol.
* The symbol visibility.
* Whether the symbol is weak or strong (i.e., non-weak).
* Whether the symbol is defined in a regular object file being included in the output, or in a shared library.
* Whether the symbol is thread local.
* Whether the symbol refers to a function or a variable.

The goal of symbol resolution is to determine the final value of the symbol. After all symbols are resolved, we should know the specific object file or shared library which defines the symbol, and we should know the symbol’s type, size, etc. It is possible that some symbols will remain undefined after all the symbol tables have been read; in general this is only an error if some relocation refers to that symbol.

At this point I’d like to present a simple algorithm for symbol resolution, but I don’t think I can. I’ll try to hit all the high points, though. Let’s assume that we have two symbols with the same name. Let’s call the symbol we saw first A and the new symbol B. (I’m going to ignore symbol visibility in the algorithm below; the effects of visibility should be obvious, I hope.)

* If A has a version:
  * If B has a version different from A, they are actually different symbols.
  * If B has the same version as A, they are the same symbol; carry on.
  * If B does not have a version, and A is the default version of the symbol, they are the same symbol; carry on.
  * Otherwise B is probably a different symbol. But note that if A and B are both undefined references, then it is possible that A refers to the default version of the symbol but we don’t yet know that. In that case, if B does not have a version, A and B really are the same symbol. We can’t tell until we see the actual definition.
* If A does not have a version:
  * If B does not have a version, they are the same symbol; carry on.
  * If B has a version, and it is the default version, they are the same symbol; carry on.
  * Otherwise, B is probably a different symbol, as above.
* If A is thread local and B is not, or vice-versa, then we have an error.
* If A is an undefined reference:
  * If B is an undefined reference, then we can complete the resolution, and more or less ignore B.
  * If B is a definition or a common symbol, then we can resolve A to B.
* If A is a strong definition in an object file:
  * If B is an undefined reference, then we resolve B to A.
  * If B is a strong definition in an object file, then we have a multiple definition error.
  * If B is a weak definition in an object file, then A overrides B. In effect, B is ignored.
  * If B is a common symbol, then we treat B as an undefined reference.
  * If B is a definition in a shared library, then A overrides B. The dynamic linker will change all references to B in the shared library to refer to A instead.
* If A is a weak definition in an object file, we act just like the strong definition case, with one exception: if B is a strong definition in an object file. In the original SVR4 linker, this case was treated as a multiple definition error. In the Solaris and GNU linkers, this case is handled by letting B override A.
* If A is a common symbol in an object file:
  * If B is a common symbol, we set the size of A to be the maximum of the size of A and the size of B, and then treat B as an undefined reference.
  * If B is a definition in a shared library with function type, then A overrides B (this oddball case is required to correctly handle some Unix system libraries).
  * Otherwise, we treat A as an undefined reference.
* If A is a definition in a shared library, then if B is a definition in a regular object (strong or weak), it overrides A. Otherwise we act as though A were defined in an object file.
* If A is a common symbol in a shared library, we have a funny case. Symbols in shared libraries must have addresses, so they can’t be common in the same sense as symbols in an object file. But ELF does permit symbols in a shared library to have the type STT\_COMMON (this is a relatively recent addition). For purposes of symbol resolution, if A is a common symbol in a shared library, we still treat it as a definition, unless B is also a common symbol. In the latter case, B overrides A, and the size of B is set to the maximum of the size of A and the size of B. I hope I got all that right.

More tomorrow, assuming the Internet connection holds up.
