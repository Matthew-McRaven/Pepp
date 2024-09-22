# Version [0.7.3](https://github.com/Matthew-McRaven/Pepp/releases/v0.7.3) -- Unreleased

This version includes bug fixes, a new changelog viewer in the Help mode, and tweaks to the Welcome screen.

## Added

 - Add changelog to "Help" mode. See [#612]()
 - Add Symbol Table Viewer to Debugger. See [#603]()

## Changed

 - Replace icons on "Welcome" mode cards with abstraction-specific artwork. See [#600]()
 - In "Welcome" mode, create uniform grid layout for abstractions and levels. See [#617]()
 - Increase size of icons in application toolbar. See [#615]()
 - Display project's architecture and abstraction in tab control. See [#619]()

## Fixed

 - Prevent "Build > Format Object Code" from deleting all object code in some cases. See [#616]()
 - In WASM builds, use our Pepp logo instead of Qt's default logo. See [#620]()


# Version [0.7.2](https://github.com/Matthew-McRaven/Pepp/releases/v0.7.2) -- Released 2024-09-14

This is the first version of the application to deploy in WASM and be accessible externally. Stan and I will rapidly iterate from here.

## Added

 - Added Web Assembly builds, deployed regularly to https://compsys-pep.com/. See [#585]()
 - Enable projects for "Pep/10, Asmb3, bare metal". See [#606]()
 - In assembly source editor, style text using theme's italic/bold settings. See [#601]()
 - Add menu entry "Build > Assemble then Load Object Code" . See [#587]()

## Changed

 - Import syntax highlighting colors from Pep/9. See [#586]()
 - On menu items: "Build > Execute", or "Debug > Start Debugging", switch active mode to "Debugger". See [#586]()
 - Remove alternate column color bands from Memory Hex Dump. See [#599]()

## Fixed

 - Use system's colors in native menu bars
 - Clear debugger's CPU & Memory on start of new simulation. See [#587]()
 - Fixed higlighting of mode buttons on application startup. See [#586]()
 - Hide menu entry "Build > Format Object Code" when not in ISA3 project. See [#593]()
 - Reduce vertical clipping in debugger's CPU pane. See [#604]()

## Optimization

 - Improve Web Assembly download speeds by reducing build size. See [#580]()


# Version [0.6.0](https://github.com/Matthew-McRaven/Pepp/releases/v0.6.0) -- Released 2024-08-28

This release contains changes to eventually allow deploying to a webpage.

## Added

 - Added ability to filter help items to a selected abstraction & architecture. See [#572]()

## Changed

 - Changed default monospace font to Courier Prime. See [#578]()

## Fixed

 - Menu entries to clear CPU/Memory. See [#587]()
 - Re-enabled ability to view licenses of included Products. See [#586]()
