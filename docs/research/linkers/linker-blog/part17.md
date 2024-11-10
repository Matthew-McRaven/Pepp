# Part 17: Warning Symbols

The GNU linker supports a weird extension to ELF used to issue warnings when symbols are referenced at link time. This was originally implemented for a.out using a special symbol type. For ELF, I implemented it using a special section name.

If you create a section named .gnu.warning.SYMBOL, then if and when the linker sees an undefined reference to SYMBOL, it will issue a warning. The warning is triggered by seeing an undefined symbol with the right name in an object file. Unlike the warning about an undefined symbol, it is not triggered by seeing a relocation entry. The text of the warning is simply the contents of the .gnu.warning.SYMBOL section.

The GNU C library uses this feature to warn about references to symbols like gets which are required by standards but are generally considered to be unsafe. This is done by creating a section named .gnu.warning.gets in the same object file which defines gets.

The GNU linker also supports another type of warning, triggered by sections named .gnu.warning (without the symbol name). If an object file with a section of that name is included in the link, the linker will issue a warning. Again, the text of the warning is simply the contents of the .gnu.warning section. I don’t know if anybody actually uses this feature.

Short entry today, more tomorrow.
