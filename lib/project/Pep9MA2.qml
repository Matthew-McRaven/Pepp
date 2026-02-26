import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/qt/qml/edu/pepp/text/editor" as Text
import "qrc:/qt/qml/edu/pepp/memory/hexdump" as Memory
import "qrc:/qt/qml/edu/pepp/memory/io" as IO
import "qrc:/qt/qml/edu/pepp/cpu" as Cpu
import "qrc:/qt/qml/edu/pepp/utils" as Utils
import "qrc:/qt/qml/edu/pepp/cpu/ma2" as MA2
import "qrc:/qt/qml/edu/pepp/text/view" as OC
import edu.pepp 1.0
import com.kdab.dockwidgets 2.0 as KDDW

FocusScope {
    id: wrapper
    required property var project
    required property var actions
    required property string mode
    // Please only use inside modeVisibilityChange.
    property string previousMode: ""
    // WASM version's active focus is broken with docks.
    required property bool isActive
    property bool needsDock: true
    property var widgets: [dock_micro, dock_cpu, dock_hexdump, dock_object]

    focus: true
    NuAppSettings {
        id: settings
    }
    function syncEditors() {
        project ? save() : null;
    }
    onModeChanged: modeVisibilityChange()
    // The mode may have changed while the window was not active.
    onIsActiveChanged: modeVisibilityChange()

    function modeVisibilityChange() {
        if (!isActive) {
            // If the window is not active / visible, restore does not work correctly, causing #974.
            return;
        } else if (needsDock) {
            // Don't allow triggering before initial docking, otherwise the layout can be 1) slow and 2) wrong.
            return;
        } else if (!(mode === "editor" || mode === "debugger")) {
            return;
        } else if (previousMode === mode) {
            // Do not attempt to lay out if mode has not changed. Possible if window was inactive.
            return;
        }

        if (previousMode)
            layoutSaver.saveToFile(`${previousMode}-${dockWidgetArea.uniqueName}.json`);
        // Only use the visibility model when restoring for the first time.
        if (!layoutSaver.restoreFromFile(`${mode}-${dockWidgetArea.uniqueName}.json`)) {
            for (const x of widgets) {
                // visibility model preserves user changes within a mode.
                const visible = x.visibility[mode];
                if (visible && !x.isOpen)
                    x.open();
                else if (!visible && x.isOpen)
                    x.close();
            }
        }
        previousMode = mode;
    }

    // Call when the height, width have been finalized.
    // Otherwise, we attempt to layout when height/width == 0, and all our requests are ignored.
    function dock() {
        const StartHidden = 1;
        const PreserveCurrent = 2;

        const total_height = parent.height;
        const code_width = 600;
        const memory_width = 400;
        const cpu_width = parent.width - memory_width - code_width;
        const microobject_height = 400;
        dockWidgetArea.addDockWidget(dock_cpu, KDDW.KDDockWidgets.Location_OnLeft, null, Qt.size(cpu_width, total_height));
        dockWidgetArea.addDockWidget(dock_hexdump, KDDW.KDDockWidgets.Location_OnLeft, dock_cpu, Qt.size(memory_width, total_height));
        dockWidgetArea.addDockWidget(dock_micro, KDDW.KDDockWidgets.Location_OnRight, dock_cpu, Qt.size(code_width, total_height - microobject_height));
        dockWidgetArea.addDockWidget(dock_object, KDDW.KDDockWidgets.Location_OnBottom, dock_micro, Qt.size(code_width, microobject_height));
        wrapper.needsDock = Qt.binding(() => false);
        modeVisibilityChange();
        // WASM version doesn't seem to give focus to editor without giving focus to something else first.
        // Without this workaround the text editor will not receive focus on subsequent key presses.
        if (PlatformDetector.isWASM)
            onLoadFocusHelper.forceActiveFocus();
        // Delay giving focus to editor until the next frame. Any editor that becomes visible without being focused will be incorrectly painted
        Qt.callLater(() => {
            onLoadFocusHelper.visible = false
            microEdit.forceEditorFocus()
        });
        for (const x of widgets) {
            x.needsAttention = false;
        }
    }

    Component.onCompleted: {
        project.markedClean.connect(wrapper.markClean);
        project.errorsChanged.connect(displayErrors);
        project.editorAction.connect(microEdit.editor.onLineAction)
        microEdit.onDirtiedChanged.connect(wrapper.markDirty);
    }

    signal requestModeSwitchTo(string mode)
    // Must be called when the project in the model is marked non-dirty
    function markClean() {
        microEdit.dirtied = false;
    }
    function markDirty() {
        if (microEdit.dirtied)
            project.markDirty();
    }

    function displayErrors() {
        microEdit.addEOLAnnotations(project.microassemblerErrors);
    }

    function save() {
        // Supress saving messages when there is no project.
        if (project)
            project.microcodeText = microEdit.text;
    }
    FontMetrics {
        id: editorFM
        font: settings.extPalette.baseMono.font
    }
    // If editor is given focus immediately in WASM, the editor is treated as RO.
    // So, we nee a dummy component which is given focus on load, so we can give focus to the real editor on the next frame.
    TextField {
        id: onLoadFocusHelper
        anchors {
            top: parent.top
            left: parent.left
        }
        width: 70
        placeholderText: "Grab Focus"
    }
    KDDW.DockingArea {
        id: dockWidgetArea
        KDDW.LayoutSaver {
            id: layoutSaver
        }
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        // Need application-wide unique ID, otherwise opening a new project will confuse the global name resolution algorithm.
        // TODO: Not gauranteed to be unique, but should be good enough for our purposes.
        uniqueName: Math.ceil(Math.random() * 1000000000).toString(16)
        KDDW.DockWidget {
            id: dock_micro
            title: "Microcode"
            uniqueName: `Microcode-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": true
            }
            Text.ScintillaMicroEdit {
                id: microEdit
                anchors.fill: parent
                editorFont: editorFM.font
                // text is only an initial binding, the value diverges from there.
                text: project.microcodeText ?? ""
                language: project.lexerLanguage ?? ""
                cycleNumbers: project.cycleNumbers ?? null
            }
        }
        KDDW.DockWidget {
            id: dock_cpu

            title: "CPU"
            uniqueName: `CPU-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": true
            }
            MA2.DatapathPainter {
                anchors.fill: parent
                clip: true
                property size kddockwidgets_min_size: Qt.size(800, 600)
                which: project.renderingType
            }
        }
        KDDW.DockWidget {
            id: dock_object

            title: "Object Code"
            uniqueName: `Object-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            OC.MicroObjectView {
                id: micro_object
                anchors.fill: parent
                property size kddockwidgets_min_size: Qt.size(200, 400)
                microcode: project?.microcode ?? null
            }
        }
        KDDW.DockWidget {
            id: dock_hexdump

            title: "Memory Dump"
            uniqueName: `MemoryDump-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            Loader {
                id: loader
                anchors.fill: parent
                Component.onCompleted: {
                    const props = {
                        "memory": project.memory,
                        "mnemonics": project.mnemonics,
                        "bytesPerRow": 4
                    };
                    // Construction sets current address to 0, which propogates back to project.
                    // Must reject changes in current address until component is fully rendered.
                    con.enabled = false;
                    setSource("qrc:/qt/qml/edu/pepp/memory/hexdump/MemoryDump.qml", props);
                }
                asynchronous: true
                clip: true
                property size kddockwidgets_min_size: Qt.size(200, 200)
                onLoaded: {
                    loader.item.scrollToAddress(project.currentAddress);
                    con.enabled = true;
                }
                Connections {
                    id: con
                    enabled: false
                    target: loader.item
                    function onCurrentAddressChanged() {
                        project.currentAddress = loader.item.currentAddress;
                    }
                }
            }
        }
    }

    // Only enable binding from the actions to this project if this project is focused.
    Connections {
        enabled: wrapper.activeFocus || wrapper.isActive
        target: wrapper.actions.debug.start
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
    Connections {
        enabled: wrapper.activeFocus || wrapper.isActive
        target: wrapper.actions.build.execute
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
}
