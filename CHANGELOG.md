# Version [0.7.5](https://github.com/Matthew-McRaven/Pepp/releases/v0.7.5) -- Unreleased

This version includes improved debugging features

## Changed

 - Removed Pep/10 OS's loader and diskIn port. See [#666](https://github.com/Matthew-McRaven/Pepp/issues/666)
 - Add stack trace to "Debugger" mode, containing views of globals, heap, and stack. See [#660](https://github.com/Matthew-McRaven/Pepp/issues/660)


# Version [0.7.4](https://github.com/Matthew-McRaven/Pepp/releases/v0.7.4) -- Released 2024-11-08

This version includes an initial implementation of Pep/9

## Added

 - Added Pep/9 projects for ISA3 and Asmb5. See [#655](https://github.com/Matthew-McRaven/Pepp/issues/655)
 - Added step over/into/out for ISA and ASMB5. See [#510](https://github.com/Matthew-McRaven/Pepp/issues/510)

## Changed

 - Made custom themes persistent between sessions in WASM
 - Re-order "Welcome" mode cards to match order of presentation in text. See [#629](https://github.com/Matthew-McRaven/Pepp/issues/629)
 - Remove banding for "Help" rows. See [#630](https://github.com/Matthew-McRaven/Pepp/issues/630)
 - Update to Qt 6.8. See [#649](https://github.com/Matthew-McRaven/Pepp/issues/649)
 - In "Welcome" mode, split double-wide Mc2 into two labeled columns. See [#650](https://github.com/Matthew-McRaven/Pepp/issues/650)

## Fixed

 - In assembler symbol tables, differentiate between user program and OS symbols. See [#634](https://github.com/Matthew-McRaven/Pepp/issues/634)
 - Only show relevant changes in "What's New" startup dialog. See [#638](https://github.com/Matthew-McRaven/Pepp/issues/638)


# Version [0.7.3](https://github.com/Matthew-McRaven/Pepp/releases/v0.7.3) -- Released 2024-09-21

This version includes bug fixes, a new changelog viewer in the Help mode, and tweaks to the Welcome screen.

## Added

 - Add changelog to "Help" mode. See [#612](https://github.com/Matthew-McRaven/Pepp/issues/612)
 - Add "What's New" dialog on startup, highlighting changes since the app was last used. See [#627](https://github.com/Matthew-McRaven/Pepp/issues/627)
 - Add Symbol Table Viewer to Debugger. See [#603](https://github.com/Matthew-McRaven/Pepp/issues/603)

## Changed

 - Replace icons on "Welcome" mode cards with abstraction-specific artwork. See [#600](https://github.com/Matthew-McRaven/Pepp/issues/600)
 - In "Welcome" mode, create uniform grid layout for abstractions and levels. See [#617](https://github.com/Matthew-McRaven/Pepp/issues/617)
 - Increase size of icons in application toolbar. See [#615](https://github.com/Matthew-McRaven/Pepp/issues/615)
 - Display project's architecture and abstraction in tab control. See [#619](https://github.com/Matthew-McRaven/Pepp/issues/619)
 - In "Welcome" mode, add meaningful descriptions to enabled cards. See [#624](https://github.com/Matthew-McRaven/Pepp/issues/624)

## Fixed

 - Prevent "Build > Format Object Code" from deleting all object code in some cases. See [#616](https://github.com/Matthew-McRaven/Pepp/issues/616)
 - In WASM builds, use our Pepp logo instead of Qt's default logo. See [#620](https://github.com/Matthew-McRaven/Pepp/issues/620)


# Version [0.7.2](https://github.com/Matthew-McRaven/Pepp/releases/v0.7.2) -- Released 2024-09-14

This is the first version of the application to deploy in WASM and be accessible externally. Stan and I will rapidly iterate from here.

## Added

 - Added Web Assembly builds, deployed regularly to https://compsys-pep.com/. See [#585](https://github.com/Matthew-McRaven/Pepp/issues/585)
 - Enable projects for "Pep/10, Asmb3, bare metal". See [#606](https://github.com/Matthew-McRaven/Pepp/issues/606)
 - In assembly source editor, style text using theme's italic/bold settings. See [#601](https://github.com/Matthew-McRaven/Pepp/issues/601)
 - Add menu entry "Build > Assemble then Load Object Code" . See [#587](https://github.com/Matthew-McRaven/Pepp/issues/587)

## Changed

 - Import syntax highlighting colors from Pep/9. See [#586](https://github.com/Matthew-McRaven/Pepp/issues/586)
 - On menu items: "Build > Execute", or "Debug > Start Debugging", switch active mode to "Debugger". See [#586](https://github.com/Matthew-McRaven/Pepp/issues/586)
 - Remove alternate column color bands from Memory Hex Dump. See [#599](https://github.com/Matthew-McRaven/Pepp/issues/599)

## Fixed

 - Use system's colors in native menu bars
 - Clear debugger's CPU & Memory on start of new simulation. See [#587](https://github.com/Matthew-McRaven/Pepp/issues/587)
 - Fixed higlighting of mode buttons on application startup. See [#586](https://github.com/Matthew-McRaven/Pepp/issues/586)
 - Hide menu entry "Build > Format Object Code" when not in ISA3 project. See [#593](https://github.com/Matthew-McRaven/Pepp/issues/593)
 - Reduce vertical clipping in debugger's CPU pane. See [#604](https://github.com/Matthew-McRaven/Pepp/issues/604)

## Optimization

 - Improve Web Assembly download speeds by reducing build size. See [#580](https://github.com/Matthew-McRaven/Pepp/issues/580)


# Version [0.6.0](https://github.com/Matthew-McRaven/Pepp/releases/v0.6.0) -- Released 2024-08-28

This release contains changes to eventually allow deploying to a webpage.

## Added

 - Added ability to filter help items to a selected abstraction & architecture. See [#572](https://github.com/Matthew-McRaven/Pepp/issues/572)

## Changed

 - Changed default monospace font to Courier Prime. See [#578](https://github.com/Matthew-McRaven/Pepp/issues/578)

## Fixed

 - Menu entries to clear CPU/Memory. See [#587](https://github.com/Matthew-McRaven/Pepp/issues/587)
 - Re-enabled ability to view licenses of included Products. See [#586](https://github.com/Matthew-McRaven/Pepp/issues/586)
