# Part 11: Archive format

Archives are a traditional Unix package format. They are created by the ar program, and they are normally named with a .a extension. Archives are passed to a Unix linker with the -l option.

Although the ar program is capable of creating an archive from any type of file, it is normally used to put object files into an archive. When it is used in this way, it creates a symbol table for the archive. The symbol table lists all the symbols defined by any object file in the archive, and for each symbol indicates which object file defines it. Originally the symbol table was created by the ranlib program, but these days it is always created by ar by default (despite this, many Makefiles continue to run ranlib unnecessarily).

When the linker sees an archive, it looks at the archive’s symbol table. For each symbol the linker checks whether it has seen an undefined reference to that symbol without seeing a definition. If that is the case, it pulls the object file out of the archive and includes it in the link. In other words, the linker pulls in all the object files which defines symbols which are referenced but not yet defined.

This operation repeats until no more symbols can be defined by the archive. This permits object files in an archive to refer to symbols defined by other object files in the same archive, without worrying about the order in which they appear.

Note that the linker considers an archive in its position on the command line relative to other object files and archives. If an object file appears after an archive on the command line, that archive will not be used to defined symbols referenced by the object file.

In general the linker will not include archives if they provide a definition for a common symbol. You will recall that if the linker sees a common symbol followed by a defined symbol with the same name, it will treat the common symbol as an undefined reference. That will only happen if there is some other reason to include the defined symbol in the link; the defined symbol will not be pulled in from the archive.

There was an interesting twist for common symbols in archives on old a.out-based SunOS systems. If the linker saw a common symbol, and then saw a common symbol in an archive, it would not include the object file from the archive, but it would change the size of the common symbol to the size in the archive if that were larger than the current size. The C library relied on this behaviour when implementing the stdin variable.

My next posting should be on Monday.
