# Pass Ordering

* `asm.peg.parseRoot`
    * Can now call `asm.visit.formatSource`
* `asm.visit.insertMacroSubtrees`
* `asm.visit.pushDownSymbols` Move symbols attached to macros to the first addressable line of code.
* `asm.visit.flattenMacros` Macro the multi-level macro tree into a single level
* `asm.visit.validateSemantics` (TODO) Validate addressing modes, argument sizes & types , symbol usage.
* `asm.visit.normalize` (TODO) Fix casing on nodes.
* `asm.visit.extractSymbols` Extract all symbol definitions and symbol references into the node's symbol table.
* `asm.visit.setTreeAddresses`
    * Can generate unpatched hex code, unpatched formatted listing, unpatched symbol table
* `asm.visit.toElf` (TODO) Pack symbol tables, code sections, relocation & debug information into an ELF file.
* Invoke the linker with all ELF files
    * Can generate patched hex code, patched formatted listing, patch symbol table