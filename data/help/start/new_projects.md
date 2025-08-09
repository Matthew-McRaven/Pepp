# Creating a New Project
Upon opening the application, you will be greeted with the **Welcome** screen, which will guide you through project creation.
A **project** is a self-contained workspace that holds your source code and the tools needed to edit, simulate, and debug that program.

You can create a new project from the **Welcome** mode by selecting a combination of textbook edition and level of abstraction.
Each button opens a fresh editor and simulator environment tailored to your selection.
[Image of welcome screen with higlights]()

You can have multiple projects open at once.
Each is simulated, edited, and debugged independently.
You can return to the **Welcome** screen at any time (see [Switching Modes]()) to start another project.


> **Note**: After creating a project, you cannot change its type. To switch architectures or levels of abstraction, create a new project.

## Choosing an Edition
The edition selector at the top of the screen corresponds to editions of *Computer Systems*; you should choose the one matching your course materials.
If you are an independent student, we recommend choosing the sixth edition.

Only one edition can be active at a time.
Selecting an edition updates the available project types listed inside of buttons below.

## Selecting a Project Type
Each button in the grid represents a specific architecture and level of abstraction.
When you choose a project type, the new environment will contain the editors, instruction set, and debugger features appropriate for that configuration.
For details of the available features in each project type, see [Editor Mode] and [Debugger Mode].

Some sample project types are:
* **Pep/10, ISA3, Bare Metal** for low-level programming object-code programming without an operating system. It features a hex editor for object code and streamlined debugging facilities for simple programs.
* **Pep/10, Asmb5, Full OS** for writing translations of C programs to assembly language or writing assembly programs with system calls. It features a source and listing pane, capable of [advanced stepping modes]() and simulating OS interactions.
* **RISC-V, Asmb3, Bare Metal** for writing assembly language in the RISC-V instruction set without out operating system support.

# Other Ways to Create a Project
You can also start a new project by:
* Selecting **File → New** (empty project).
* Opening a file with **File → Open** (project loads with the file’s content).
* Copying an example from the help system (project loads with the example’s code).

