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
enum class Command {
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
  Run,        // ()

  // Debug
  DebugStart,    // ()
  DebugPause,    // ()
  DebugContinue, // ()
  DebugStop,     // ()
  DebugStep,     // ()
  DebugStepInto, // ()
  DebugStepOver, // ()
  DebugStepOut,  // ()

  // Simulator
  ClearCPU,    // ()
  ClearMemory, // ()

  // Help
  About, // ()

};
class UTILS_EXPORT CommandHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Command);

public:
  CommandHelper(QObject *parent = nullptr);
  Q_ENUM(utils::Command);
};

}; // namespace utils
