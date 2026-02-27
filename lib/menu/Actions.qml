import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

QtObject {
    required property var window
    required property var project
    required property var settings
    property bool dark: window.palette.text.hslLightness < 0.5
    function updateNativeText(obj) {
        obj.nativeText = Qt.binding(() => SequenceConverter.toNativeText(obj.shortcut));
    }

    readonly property var file: QtObject {
        readonly property var new_: Action {
            property string nativeText: ""
            text: qsTr("&New")
            onTriggered: window.onNew()
            icon.source: `image://icons/file/new${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.New
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var open: Action {
            property string nativeText: ""
            text: qsTr("&Open...")
            onTriggered: window.onOpenDialog()
            icon.source: `image://icons/file/open${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.Open
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var clearRecents: Action {
            property string nativeText: ""
            text: qsTr("&Clear Recent Files")
            onTriggered: settings.general.clearRecentFiles()
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var save: Action {
            property string nativeText: ""
            text: qsTr("&Save")
            icon.source: `image://icons/file/save${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.Save
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var saveAs: Action {
            property string nativeText: ""
            text: qsTr("Save as")
            icon.source: `image://icons/file/save${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.SaveAs
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var print_: Action {
            text: qsTr("&Print")
            onTriggered: console.log(this.text)
            // Use blank icon to force menu items to line up.
            icon.source: "image://icons/blank.svg"
        }
        readonly property var closeAll: Action {
            text: qsTr("Close All")
            onTriggered: window.onCloseAllProjects(false)
            icon.source: "image://icons/blank.svg"
        }
        readonly property var closeAllButCurrent: Action {
            text: qsTr("Close All Except Current")
            onTriggered: window.onCloseAllProjects(true)
            icon.source: "image://icons/blank.svg"
        }
        readonly property var quit: Action {
            property string nativeText: ""
            text: qsTr("&Quit")
            onTriggered: window.onQuit()
            icon.source: "image://icons/blank.svg"
            shortcut: "Ctrl+Q"
            onShortcutChanged: updateNativeText(this)
        }
    }

    readonly property var edit: QtObject {
        readonly property var undo: Action {
            property string nativeText: ""
            text: qsTr("&Undo")
            icon.source: `image://icons/file/undo${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.Undo
            onShortcutChanged: updateNativeText(this)
            enabled: !!activeFocusItem && !!activeFocusItem["undo"] && (!activeFocusItem.readOnly ?? true)
            onTriggered: activeFocusItem.undo()
        }
        readonly property var redo: Action {
            property string nativeText: ""
            text: qsTr("&Redo")
            icon.source: `image://icons/file/redo${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.Redo
            onShortcutChanged: updateNativeText(this)
            enabled: !!activeFocusItem && !!activeFocusItem["redo"] && (!activeFocusItem.readOnly ?? true)
            onTriggered: activeFocusItem.redo()
        }
        readonly property var copy: Action {
            property string nativeText: ""
            text: qsTr("&Copy")
            icon.source: `image://icons/file/copy${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.Copy
            onShortcutChanged: updateNativeText(this)
            enabled: !!activeFocusItem && !!activeFocusItem["copy"]
            onTriggered: activeFocusItem.copy()
        }
        readonly property var cut: Action {
            property string nativeText: ""
            text: qsTr("Cu&t")
            icon.source: `image://icons/file/cut${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.Cut
            onShortcutChanged: updateNativeText(this)
            enabled: !!activeFocusItem && !!activeFocusItem["cut"] && (!activeFocusItem.readOnly ?? true)
            onTriggered: activeFocusItem.cut()
        }
        readonly property var paste: Action {
            property string nativeText: ""
            text: qsTr("&Paste")
            icon.source: `image://icons/file/paste${dark ? '' : '_dark'}.svg`
            shortcut: StandardKey.Paste
            onShortcutChanged: updateNativeText(this)
            enabled: !!activeFocusItem && !!activeFocusItem["paste"] && (!activeFocusItem.readOnly ?? true)
            onTriggered: activeFocusItem.paste()
        }
        readonly property var prefs: Action {
            text: qsTr("Pr&eferences")
            icon.source: `image://icons/file/settings${dark ? '' : '_dark'}.svg`
        }
        readonly property var clearEditorErrors: Action {
            text: qsTr("Clear Editor Errors")
            icon.source: `image://icons/blank.svg`
            enabled: (project?.onClearEditorErrors ?? undefined) !== undefined
            onTriggered: {
                if (project.onClearEditorErrors)
                    project.onClearEditorErrors();
            }
        }
    }

    readonly property var build: QtObject {
        readonly property var formatObject: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.LoadObject
            property string nativeText: ""
            onTriggered: {
                window.syncEditors();
                project.onFormatObject();
            }
            text: "&Format Object Code"
            icon.source: "image://icons/blank.svg"
            shortcut: ["Ctrl+Shift+F"]
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var loadObject: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.LoadObject
            property string nativeText: ""
            onTriggered: {
                window.syncEditors();
                project.onLoadObject();
            }
            text: qsTr("&Load Object Code")
            icon.source: `image://icons/build/flash${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            shortcut: "Ctrl+Shift+L"
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var assemble: Action {
            enabled: project?.onAssemble !== undefined
            property string nativeText: ""
            onTriggered: {
                // New editor does not lose focus before "assemble" is triggered, so we must save manually.
                window.syncEditors();
                project.onAssemble();
            }
            text: qsTr("&Assemble")
            icon.source: `image://icons/build/build${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            shortcut: "Ctrl+Shift+B"
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var assembleThenLoad: Action {
            enabled: project?.onAssemble !== undefined
            property string nativeText: ""
            onTriggered: {
                // New editor does not lose focus before "assemble" is triggered, so we must save manually.
                window.syncEditors();
                project.onAssembleThenLoad();
            }
            text: qsTr("Assemble && &Load Object Code")
            icon.source: `image://icons/build/flash${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            shortcut: "Ctrl+Shift+L"
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var assembleThenFormat: Action {
            enabled: project?.onAssembleThenFormat !== undefined
            property string nativeText: ""
            onTriggered: {
                // New editor does not lose focus before "assemble" is triggered, so we must save manually.
                window.syncEditors();
                project.onAssembleThenFormat();
                project.overwriteEditors();
            }
            text: qsTr("&Format Source Code")
            // Use blank icon to force menu items to line up.
            icon.source: "image://icons/blank.svg"
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var microAssemble: Action {
            enabled: project?.onMicroAssemble !== undefined
            property string nativeText: ""
            onTriggered: {
                // New editor does not lose focus before "assemble" is triggered, so we must save manually.
                window.syncEditors();
                project.onMicroAssemble();
            }
            text: qsTr("&Microassemble")
            icon.source: `image://icons/build/build${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var microAssembleThenFormat: Action {
            enabled: project?.onMicroAssembleThenFormat !== undefined
            property string nativeText: ""
            onTriggered: {
                // New editor does not lose focus before "assemble" is triggered, so we must save manually.
                window.syncEditors();
                project.onMicroAssembleThenFormat();
                project.overwriteEditors();
            }
            text: qsTr("&Format Microcode")
            // Use blank icon to force menu items to line up.
            icon.source: "image://icons/blank.svg"
            onShortcutChanged: updateNativeText(this)
        }

        readonly property var execute: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Execute
            property string nativeText: ""
            onTriggered: {
                // New editor does not lose focus before "assemble" is triggered, so we must save manually.
                window.syncEditors();
                project.onExecute();
            }
            text: qsTr("&Run")
            icon.source: `image://icons/debug/start_normal${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            shortcut: "Ctrl+Shift+R"
            onShortcutChanged: updateNativeText(this)
        }
    }

    readonly property var debug: QtObject {
        readonly property var start: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Start
            property string nativeText: ""
            onTriggered: {
                // New editor does not lose focus before "assemble" is triggered, so we must save manually.
                window.syncEditors();
                project.onDebuggingStart();
            }
            text: qsTr("Start &Debugging")
            icon.source: `image://icons/debug/start_debug${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            shortcut: "Ctrl+D"
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var continue_: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Continue
            text: qsTr("&Continue Debugging")
            onTriggered: project.onDebuggingContinue()
            icon.source: `image://icons/debug/continue_debug${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
        }
        readonly property var pause: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Pause
            property string nativeText: ""
            onTriggered: project.onDebuggingPause()
            text: qsTr("I&nterrupt Debugging")
            icon.source: `image://icons/debug/pause${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            shortcut: "Ctrl+."
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var stop: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Stop
            text: qsTr("S&top Debugging")
            onTriggered: project.onDebuggingStop()
            icon.source: `image://icons/debug/stop_debug${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
        }
        readonly property var step: Action {
            enabled: project?.allowedSteps & StepEnableFlags.Step
            property string nativeText: ""
            onTriggered: {
                if(project.onISAStep)
                    project.onISAStep();
                else if(project.onMAStep)
                    project.onMAStep();
                else if(project.onStep)
                    project.onStep();
            }
            text: qsTr("&Single Step")
            icon.source: `image://icons/debug/step_normal${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
            shortcut: ["Ctrl+Return"]
            onShortcutChanged: updateNativeText(this)
        }
        readonly property var stepOver: Action {
            enabled: project?.allowedSteps & StepEnableFlags.StepOver
            text: qsTr("Step O&ver")
            onTriggered: project.onISAStepOver()
            icon.source: `image://icons/debug/step_over${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
        }
        readonly property var stepInto: Action {
            enabled: project?.allowedSteps & StepEnableFlags.StepInto
            text: qsTr("Step &Into")
            onTriggered: project.onISAStepInto()
            icon.source: `image://icons/debug/step_into${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
        }
        readonly property var stepOut: Action {
            enabled: project?.allowedSteps & StepEnableFlags.StepOut
            text: qsTr("Step &Out")
            onTriggered: project.onISAStepOut()
            icon.source: `image://icons/debug/step_out${enabled ? '' : '_disabled'}${dark ? '' : '_dark'}.svg`
        }
        readonly property var removeAllBreakpoints: Action {
            onTriggered: {
                if(project.onISARemoveAllBreakpoints)
                    project.onISARemoveAllBreakpoints();
                else if(project.onMARemoveAllBreakpoints)
                    project.onMARemoveAllBreakpoints();
                else if(project.onRemoveAllBreakpoints)
                     project.onRemoveAllBreakpoints();
            }
            text: qsTr("&Remove All Breakpoints")
            icon.source: "image://icons/blank.svg"
        }
    }

    readonly property var view: QtObject {
        readonly property var fullscreen: Action {
            text: qsTr("&Toggle Fullscreen")
            icon.source: "image://icons/blank.svg"
        }
    }

    readonly property var sim: QtObject {
        readonly property var clearCPU: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Start
            text: qsTr("Clear &CPU")
            onTriggered: project.onClearCPU()
            icon.source: "image://icons/blank.svg"
        }
        readonly property var clearMemory: Action {
            enabled: project?.allowedDebugging & DebugEnableFlags.Start
            text: qsTr("Clear &Memory")
            onTriggered: project.onClearMemory()
            icon.source: "image://icons/blank.svg"
        }
    }

    readonly property var help: QtObject {
        readonly property var about: Action {
            text: qsTr("&About")
            icon.source: "image://icons/blank.svg"
        }
        readonly property var resetSettings: Action {
            property string nativeText: ""
            text: qsTr("&Restore Default Settings")
            icon.source: "image://icons/blank.svg"
            onShortcutChanged: updateNativeText(this)
        }
    }

    readonly property var appdev: QtObject {
        readonly property var clearChangelogCache: Action {
            text: qsTr("Clear Changelog Cache")
            icon.source: "image://icons/blank.svg"
        }
        readonly property var reloadFigures: Action {
            text: qsTr("Reload Figures")
            icon.source: "image://icons/blank.svg"
        }
        readonly property var openSelftest: Action {
            text: qsTr("Open self-test GUI")
            icon.source: "image://icons/blank.svg"
        }
    }
}
