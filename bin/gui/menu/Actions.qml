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
            icon.source: "image://icons/file/new.svg"
            shortcut: StandardKey.New
        }
        readonly property var open: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Open..."
            onTriggered: window.onOpenDialog()
            icon.source: "image://icons/file/open.svg"
            shortcut: StandardKey.Open
        }
        readonly property var save: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onSaveCurrent()
            text: "&Save"
            icon.source: "image://icons/file/save.svg"
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
            icon.source: "image://icons/file/undo.svg"
            shortcut: StandardKey.Undo
        }
        readonly property var redo: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Redo"
            icon.source: "image://icons/file/redo.svg"
            shortcut: StandardKey.Redo
        }
        readonly property var copy: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Copy"
            icon.source: "image://icons/file/copy.svg"
            shortcut: StandardKey.Copy
        }
        readonly property var cut: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "Cu&t"
            icon.source: "image://icons/file/cut.svg"
            shortcut: StandardKey.Cut
        }
        readonly property var paste: Action {
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            text: "&Paste"
            icon.source: "image://icons/file/paste.svg"
            shortcut: StandardKey.Paste
        }
        readonly property var prefs: Action {
            text: "Pr&eferences"
            icon.source: "image://icons/file/settings.svg"
        }
    }

    readonly property var build: QtObject {
        readonly property var loadObject: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.LoadObject
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onLoadObject()
            text: "&Load Object Code"
            icon.source: `image://icons/build/flash${enabled ? '' : '_disabled'}.svg`
            shortcut: "Ctrl+Shift+L"
        }
        readonly property var execute: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Execute
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onExecute()
            text: "&Execute"
            icon.source: `image://icons/debug/start_normal${enabled ? '' : '_disabled'}.svg`
            shortcut: "Ctrl+Shift+R"
        }
    }

    readonly property var debug: QtObject {
        readonly property var start: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Start
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onDebuggingStart()
            text: "Start &Debugging"
            icon.source: `image://icons/debug/start_debug${enabled ? '' : '_disabled'}.svg`
            shortcut: "Ctrl+D"
        }
        readonly property var continue_: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Continue
            text: "&Continue Debugging"
            onTriggered: project.onDebuggingContinue()
            icon.source: `image://icons/debug/continue_debug${enabled ? '' : '_disabled'}.svg`
        }
        readonly property var pause: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Pause
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onDebuggingPause()
            text: "I&nterrupt Debugging"
            icon.source: `image://icons/debug/pause${enabled ? '' : '_disabled'}.svg`
            shortcut: "Ctrl+."
        }
        readonly property var stop: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Stop
            text: "S&top Debugging"
            onTriggered: project.onDebuggingStop()
            icon.source: `image://icons/debug/stop_debug${enabled ? '' : '_disabled'}.svg`
        }
        readonly property var step: Action {
            enabled: project?.allowedSteps & StepEnableFlags.Step
            property string nativeText: ""
            onShortcutChanged: nativeText = Qt.binding(
                                   () => SequenceConverter.toNativeText(
                                       this.shortcut))
            onTriggered: project.onISAStep()
            text: "&Step"
            icon.source: `image://icons/debug/step_normal${enabled ? '' : '_disabled'}.svg`
            shortcut: "Ctrl+Return"
        }
        readonly property var stepOver: Action {
            enabled: project?.allowedSteps & StepEnableFlags.StepOver
            text: "Step O&ver"
            onTriggered: project.onISAStepOver()
            icon.source: `image://icons/debug/step_over${enabled ? '' : '_disabled'}.svg`
        }
        readonly property var stepInto: Action {
            enabled: project?.allowedSteps & StepEnableFlags.StepInto
            text: "Step &Into"
            onTriggered: project.onISAStepInto()
            icon.source: `image://icons/debug/step_into${enabled ? '' : '_disabled'}.svg`
        }
        readonly property var stepOut: Action {
            enabled: project?.allowedSteps & StepEnableFlags.StepOut
            text: "Step &Out"
            onTriggered: project.onISAStepOut()
            icon.source: `image://icons/debug/step_out${enabled ? '' : '_disabled'}.svg`
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
