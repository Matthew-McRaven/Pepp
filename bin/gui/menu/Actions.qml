import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

QtObject {
    required property var window
    required property var project
    readonly property var file: QtObject {
        readonly property var new_: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&New"
            onTriggered: window.onNew()
            icon.source: "qrc:/icons/file/new.svg"
            shortcut: StandardKey.New
        }
        readonly property var open: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Open..."
            onTriggered: window.onOpenDialog()
            icon.source: "qrc:/icons/file/open.svg"
            shortcut: StandardKey.Open
        }
        readonly property var save: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onSaveCurrent()
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
            onTriggered: window.onCloseAllProjects(false)
        }
        readonly property var closeAllButCurrent: Action {
            text: "Close All Except Current"
            onTriggered: window.onCloseAllProjects(true)
        }
        readonly property var quit: Action {
            text: "&Quit"
            onTriggered: window.onQuit()
        }
    }

    readonly property var edit: QtObject {
        readonly property var undo: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Undo"
            icon.source: "qrc:/icons/file/undo.svg"
            shortcut: StandardKey.Undo
        }
        readonly property var redo: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Redo"
            icon.source: "qrc:/icons/file/redo.svg"
            shortcut: StandardKey.Redo
        }
        readonly property var copy: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Copy"
            icon.source: "qrc:/icons/file/copy.svg"
            shortcut: StandardKey.Copy
        }
        readonly property var cut: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "Cu&t"
            icon.source: "qrc:/icons/file/cut.svg"
            shortcut: StandardKey.Cut
        }
        readonly property var paste: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Paste"
            icon.source: "qrc:/icons/file/paste.svg"
            shortcut: StandardKey.Paste
        }
        readonly property var prefs: Action {
            text: "Pr&eferences"
            icon.source: "qrc:/icons/file/settings.svg"
        }
    }

    readonly property var build: QtObject {
        readonly property var loadObject: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onLoadObject()
            text: "&Load Object Code"
            icon.source: "qrc:/icons/build/flash.svg"
            shortcut: "Ctrl+Shift+L"
        }
        readonly property var execute: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onExecute()
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
            onTriggered: project.onDebuggingStart()
            text: "Start &Debugging"
            icon.source: "qrc:/icons/debug/start_debug.svg"
            shortcut: "Ctrl+D"
        }
        readonly property var continue_: Action {
            text: "&Continue Debugging"
            onTriggered: project.onDebuggingContinue()
            icon.source: "qrc:/icons/debug/continue_debug.svg"
        }
        readonly property var pause: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onDebuggingPause()
            text: "I&nterrupt Debugging"
            icon.source: "qrc:/icons/debug/pause.svg"
            shortcut: "Ctrl+."
        }
        readonly property var stop: Action {
            text: "S&top Debugging"
            onTriggered: project.onDebuggingStop()
            icon.source: "qrc:/icons/debug/stop_debug.svg"
        }
        readonly property var step: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onISAStep()
            text: "&Step"
            icon.source: "qrc:/icons/debug/step_normal.svg"
            shortcut: "Ctrl+Return"
        }
        readonly property var stepOver: Action {
            text: "Step O&ver"
            onTriggered: project.onISAStepOver()
            icon.source: "qrc:/icons/debug/step_over.svg"
        }
        readonly property var stepInto: Action {
            text: "Step &Into"
            onTriggered: project.onISAStepInto()
            icon.source: "qrc:/icons/debug/step_into.svg"
        }
        readonly property var stepOut: Action {
            text: "Step &Out"
            onTriggered: project.onISAStepOut()
            icon.source: "qrc:/icons/debug/step_out.svg"
        }
        readonly property var removeAllBreakpoints: Action {
            onTriggered: project.onISARemoveAllBreakpoints()
            text: "&Remove All Breakpoints"
        }
    }

    readonly property var view: QtObject {
        readonly property var fullscreen: Action {
            text: "&Toggle Fullscreen"
        }
    }

    readonly property var sim: QtObject {
        readonly property var clearCPU: Action {
            text: "Clear &CPU"
            onTriggered: project.onClearCPU()
        }
        readonly property var clearMemory: Action {
            text: "Clear &Memory"
            onTriggered: project.onClearMemory()
        }
    }

    readonly property var help: QtObject {
        readonly property var about: Action {
            text: "&About"
        }
    }
}
