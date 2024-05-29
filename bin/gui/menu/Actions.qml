import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

QtObject {
    required property var target
    readonly property var file: QtObject {
        readonly property var new_: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&New"
            onTriggered: target.onCommand(Commands.New, null)
            icon.source: "qrc:/icons/file/new.svg"
            shortcut: StandardKey.New
        }
        readonly property var open: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Open..."
            onTriggered: target.onCommand(Commands.OpenDialog, null)
            icon.source: "qrc:/icons/file/open.svg"
            shortcut: StandardKey.Open
        }
        readonly property var save: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.SaveCurrent, null)
            text: "&Save"
            icon.source: "qrc:/icons/file/save.svg"
            shortcut: StandardKey.Save
        }
        readonly property var print_: Action {
            text: "&Print"
            onTriggered: console.log(this.text)
        }
        readonly property var closeAll: Action {
            text: "Close All"
            onTriggered: target.onCommand(Commands.CloseAllProjects, null)
        }
        readonly property var closeAllButCurrent: Action {
            text: "Close All Except Current"
            onTriggered: target.onCommand(
                             Commands.CloseAllProjectsExceptCurrent, null)
        }
        readonly property var quit: Action {
            text: "&Quit"
            onTriggered: console.log(this.text)
        }
    }

    readonly property var edit: QtObject {
        readonly property var undo: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.EditorUndo, null)
            text: "&Undo"
            icon.source: "qrc:/icons/file/undo.svg"
            shortcut: StandardKey.Undo
        }
        readonly property var redo: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.EditorRedo, null)
            text: "&Redo"
            icon.source: "qrc:/icons/file/redo.svg"
            shortcut: StandardKey.Redo
        }
        readonly property var copy: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.EditorCopy, null)
            text: "&Copy"
            icon.source: "qrc:/icons/file/copy.svg"
            shortcut: StandardKey.Copy
        }
        readonly property var cut: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.EditorCut, null)
            text: "Cu&t"
            icon.source: "qrc:/icons/file/cut.svg"
            shortcut: StandardKey.Cut
        }
        readonly property var paste: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.EditorPaste, null)
            text: "&Paste"
            icon.source: "qrc:/icons/file/paste.svg"
            shortcut: StandardKey.Paste
        }
        readonly property var prefs: Action {
            text: "Pr&eferences"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/settings.svg"
        }
    }

    readonly property var build: QtObject {
        readonly property var loadObject: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.LoadObject, null)
            text: "&Load Object Code"
            icon.source: "qrc:/icons/build/flash.svg"
            shortcut: "Ctrl+Shift+L"
        }
        readonly property var execute: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.Execute, null)
            text: "&Execute"
            icon.source: "qrc:/icons/debug/start_normal.svg"
            shortcut: "Ctrl+Shift+R"
        }
    }

    readonly property var debug: QtObject {
        readonly property var start: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.DebugStart, null)
            text: "Start &Debugging"
            icon.source: "qrc:/icons/debug/start_debug.svg"
            shortcut: "Ctrl+D"
        }
        readonly property var continue_: Action {
            text: "&Continue Debugging"
            onTriggered: target.onCommand(Commands.DebugContinue, null)
            icon.source: "qrc:/icons/debug/continue_debug.svg"
        }
        readonly property var pause: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.DebugPause, null)
            text: "I&nterrupt Debugging"
            icon.source: "qrc:/icons/debug/pause.svg"
            shortcut: "Ctrl+."
        }
        readonly property var stop: Action {
            text: "S&top Debugging"
            onTriggered: target.onCommand(Commands.DebugStop, null)
            icon.source: "qrc:/icons/debug/stop_debug.svg"
        }
        readonly property var step: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: target.onCommand(Commands.DebugStep, null)
            text: "&Step"
            icon.source: "qrc:/icons/debug/step_normal.svg"
            shortcut: "Ctrl+Return"
        }
        readonly property var stepOver: Action {
            text: "Step O&ver"
            onTriggered: target.onCommand(Commands.DebugStepOver, null)
            icon.source: "qrc:/icons/debug/step_over.svg"
        }
        readonly property var stepInto: Action {
            text: "Step &Into"
            onTriggered: target.onCommand(Commands.DebugStepInto, null)
            icon.source: "qrc:/icons/debug/step_into.svg"
        }
        readonly property var stepOut: Action {
            text: "Step &Out"
            onTriggered: target.onCommand(Commands.DebugStepOut, null)
            icon.source: "qrc:/icons/debug/step_out.svg"
        }
        readonly property var removeAllBreakpoints: Action {
            text: "&Remove All Breakpoints"
            onTriggered: target.onCommand(Commands.RemoveAllBreakpoints, null)
        }
    }

    readonly property var view: QtObject {
        readonly property var fullscreen: Action {
            text: "&Toggle Fullscreen"
            onTriggered: target.onCommand(Commands.ToggleFullScreen, null)
        }
    }

    readonly property var sim: QtObject {
        readonly property var clearCPU: Action {
            text: "Clear &CPU"
            onTriggered: target.onCommand(Commands.ClearCPU, null)
        }
        readonly property var clearMemory: Action {
            text: "Clear &Memory"
            onTriggered: target.onCommand(Commands.ClearMemory, null)
        }
    }

    readonly property var help: QtObject {
        readonly property var about: Action {
            text: "&About"
        }
    }
}
