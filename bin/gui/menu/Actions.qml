import QtQuick
import QtQuick.Controls

QtObject {
    required property var target
    readonly property var file: QtObject {
        readonly property var new_: Action {
            text: "&New"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/new.svg"
            shortcut: StandardKey.New
        }
        readonly property var open: Action {
            text: "&Open..."
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/open.svg"
            shortcut: StandardKey.Open
        }
        readonly property var save: Action {
            text: "&Save"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/save.svg"
            shortcut: StandardKey.Save
        }
        readonly property var print_: Action {
            text: "&Print"
            onTriggered: console.log(this.text)
        }
        readonly property var closeAll: Action {
            text: "Close All"
            onTriggered: console.log(this.text)
        }
        readonly property var closeAllButCurrent: Action {
            text: "Close All Except Current"
            onTriggered: console.log(this.text)
        }
        readonly property var quit: Action {
            text: "&Quit"
            onTriggered: console.log(this.text)
        }
    }

    readonly property var edit: QtObject {
        readonly property var undo: Action {
            text: "&Undo"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/undo.svg"
            shortcut: StandardKey.Undo
        }
        readonly property var redo: Action {
            text: "&Redo"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/redo.svg"
            shortcut: StandardKey.Redo
        }
        readonly property var copy: Action {
            text: "&Copy"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/copy.svg"
            shortcut: StandardKey.Copy
        }
        readonly property var cut: Action {
            text: "Cu&t"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/file/cut.svg"
            shortcut: StandardKey.Cut
        }
        readonly property var paste: Action {
            text: "&Paste"
            onTriggered: console.log(this.text)
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
            text: "&Load Object Code"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/build/flash.svg"
            shortcut: ["Ctrl+Shift+L"]
        }
        readonly property var execute: Action {
            text: "&Execute"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/start_normal.svg"
            shortcut: ["Ctrl+Shift+R"]
        }
    }

    readonly property var debug: QtObject {
        readonly property var start: Action {
            text: "Start &Debugging"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/start_debug.svg"
            shortcut: "Ctrl+D"
        }
        readonly property var continue_: Action {
            text: "&Continue Debugging"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/continue_debug.svg"
        }
        readonly property var pause: Action {
            text: "I&nterrupt Debugging"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/pause.svg"
            shortcut: "Ctrl+."
        }
        readonly property var stop: Action {
            text: "S&top Debugging"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/stop_debug.svg"
        }
        readonly property var step: Action {
            text: "&Step"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/step_normal.svg"
            shortcut: "Ctrl+Return"
        }
        readonly property var stepOver: Action {
            text: "Step O&ver"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/step_over.svg"
        }
        readonly property var stepInto: Action {
            text: "Step &Into"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/step_into.svg"
        }
        readonly property var stepOut: Action {
            text: "Step &Out"
            onTriggered: console.log(this.text)
            icon.source: "qrc:/icons/debug/step_out.svg"
        }
        readonly property var removeAllBreakpoints: Action {
            text: "&Remove All Breakpoints"
            onTriggered: console.log(this.text)
        }
    }

    readonly property var view: QtObject {
        readonly property var fullscreen: Action {
            text: "&Toggle Fullscreen"
            onTriggered: console.log(this.text)
        }
    }

    readonly property var sim: QtObject {
        readonly property var clearCPU: Action {
            text: "Clear &CPU"
            onTriggered: console.log(this.text)
        }
        readonly property var clearMemory: Action {
            text: "Clear &Memory"
            onTriggered: console.log(this.text)
        }
    }

    readonly property var help: QtObject {
        readonly property var about: Action {
            text: "&About"
            onTriggered: console.log(this.text)
        }
    }
}
