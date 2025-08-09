# Switching Modes
Pep is a modal IDE, meaning the interface changes depending on the task you are performing.
The mode selector appears as a vertical bar of icons on the left side of the window.
The **Welcome** and **Help** modes are always visible, with additional modes avaialble depending on the selected project
[Mode image]()

## Available Modes
* **Welcome** — Create a new project, open an existing one, or select examples from the help system.
* **Help** — You are here. View the built-in help documentation, including examples that can be copied directly into a new project.
* **Editor** — Edit your project’s source or object code. This mode also includes tools for assembling and loading programs.
* **Debugger** — Simulate, step through, and inspect the execution of code in your project.

## Changing Modes
Clicking an icon immediately switches to that mode, preserving the current state of the active project.
For example, you can switch between **Editor** and **Debugger** without restarting or losing your position in the editor.
Switching modes is **global**, meaning the selected mode will be used for all open projects.

The set of available modes depends on the active project’s architecture and level of abstraction—some modes are not supported for all project types.
For example, the microcode editor's dapatath view is not avaialble at other levels of abstraction.

Some actions—such as creating a new project from the Welcome screen—automatically switch you into the **Editor** mode.
