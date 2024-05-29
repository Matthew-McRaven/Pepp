#pragma once
#include <QObject>
#include <qqmlintegration.h>
#include "utils_global.hpp"

namespace utils {
Q_NAMESPACE_EXPORT(UTILS_EXPORT)
enum class WhichPane {
  Current,
  Object = 1,
  Assembler = 2,
  Listing = 4,
};

class UTILS_EXPORT WhichPaneHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(WhichPane);

public:
  WhichPaneHelper(QObject *parent = nullptr);
  Q_ENUM(utils::WhichPane);
};

class UTILS_EXPORT CommandHelper : public QObject {
  Q_GADGET
public:
  enum class Command {
    // Qt will convert null/undefined to 0 when calling an int function.
    // Insert an entry at 0 that can never be valid to aid debugging.
    Undefined = 0,
    // (internal) Project
    CurrentMode,        // () => Mode
    CurrentProject,     // () => int index
    ProjectAbstraction, // () => Abstraction
    ProjectArch,        // () => Arch
    ListRecent,         // () => string[]
    ListModes,          // () => bits<Mode>
    ListPanes,          // () => bits<WhichPane>
    SetPaneContents,    // (WhichPane, string) => bool success
    SetActiveProject,   // (int index) => bool success

    // File
    New,                           // () => int index
    OpenDialog,                    // () => bool success
    OpenFile,                      // (string path) => string contents
    Save,                          // (WhichPane) => bool success
    SaveCurrent,                   // () => bool success, alias for Save(WhichPane::Current)
    SaveAs,                        // (WhichPane) => bool success
    Print,                         // (WhichPane) => bool success
    CloseProject,                  // (int index) => bool success
    CloseAllProjects,              // () => bool success
    CloseAllProjectsExceptCurrent, // ()
    Quit,                          // ()

    // Edit
    EditorUndo,  // ()
    EditorRedo,  // ()
    EditorCut,   // ()
    EditorCopy,  // ()
    EditorPaste, // ()
    Prefs,       // ()

    // View
    ToggleFullScreen, // ()

    // Build
    LoadObject, // ()
    Execute,    // ()

    // Debug
    DebugStart,           // ()
    DebugPause,           // ()
    DebugContinue,        // ()
    DebugStop,            // ()
    DebugStep,            // ()
    DebugStepInto,        // ()
    DebugStepOver,        // ()
    DebugStepOut,         // ()
    RemoveAllBreakpoints, // ()

    // Simulator
    ClearCPU,    // ()
    ClearMemory, // ()

    // Help
    About, // ()

  };
  Q_ENUM(Command);
  CommandHelper(QObject *parent = nullptr);
};
using Command = CommandHelper::Command;
}; // namespace utils
