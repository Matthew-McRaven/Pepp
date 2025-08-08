import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/qt/qml/edu/pepp/text/editor" as Text
import "qrc:/qt/qml/edu/pepp/memory/hexdump" as Memory
import "qrc:/qt/qml/edu/pepp/memory/io" as IO
import "qrc:/qt/qml/edu/pepp/cpu" as Cpu
import "qrc:/qt/qml/edu/pepp/utils" as Utils
import edu.pepp 1.0
import com.kdab.dockwidgets 2 as KDDW

FocusScope {
    id: wrapper
    required property var project
    required property var actions
    required property string mode
    // WASM version's active focus is broken with docks.
    required property bool isActive
    property bool needsDock: true
    focus: true
    signal requestModeSwitchTo(string mode)

    function syncEditors() {
        project ? save() : null;
    }
    onModeChanged: modeVisibilityChange()

    function modeVisibilityChange() {
        // Don't allow triggering before initial docking, otherwise the layout can be 1) slow and 2) wrong.
        if (needsDock) {
            return;
        } else if (!(mode === "editor" || mode === "debugger")) {
            return;
        }
        // visibility model preserves user changes within a mode.
        for (const x of visibilityBar.model) {
            const visible = x.visibility[mode];
            if (visible && !x.isOpen)
                x.open();
            else if (!visible && x.isOpen)
                x.close();
        }
    }
    // Must be called when the project in the model is marked non-dirty
    function markClean() {
        objEdit.dirtied = Qt.binding(() => false);
    }
    function markDirty() {
        if (objEdit.dirtied)
            project.markDirty();
    }

    // Call when the height, width have been finalized.
    // Otherwise, we attempt to layout when height/width == 0, and all our requests are ignored.
    function dock() {
        const StartHidden = 1;
        const PreserveCurrent = 2;

        const total_height = parent.height - visibilityBar.height;

        const reg_height = registers.childrenRect.height;
        const regmemcol_width = registers.implicitWidth;

        const memdump_height = total_height - registers.implicitHeight;
        const greencard_width = parent.width * .3;

        const io_height = Math.max(200, total_height * .1);

        dockWidgetArea.addDockWidget(dock_object, KDDW.KDDockWidgets.Location_OnLeft, dockWidgetArea, Qt.size(parent.width - greencard_width - regmemcol_width, parent.height - io_height));
        dockWidgetArea.addDockWidget(dock_greencard, KDDW.KDDockWidgets.Location_OnRight, dockWidgetArea, Qt.size(greencard_width, parent.height - io_height));
        dockWidgetArea.addDockWidget(dock_input, KDDW.KDDockWidgets.Location_OnBottom, dockWidgetArea, Qt.size(parent.width - regmemcol_width, io_height));
        dock_input.addDockWidgetAsTab(dock_output, StartHidden);
        dockWidgetArea.addDockWidget(dock_cpu, KDDW.KDDockWidgets.Location_OnRight, dockWidgetArea, Qt.size(regmemcol_width, reg_height));
        dockWidgetArea.addDockWidget(dock_hexdump, KDDW.KDDockWidgets.Location_OnBottom, dock_cpu, Qt.size(regmemcol_width, memdump_height));
        wrapper.needsDock = Qt.binding(() => false);
        modeVisibilityChange();
    }

    Component.onCompleted: {
        project.charInChanged.connect(() => batchInput.setInput(project.charIn));
        objEdit.editingFinished.connect(save);
        objEdit.forceActiveFocus();
        project.markedClean.connect(wrapper.markClean);
        objEdit.onDirtiedChanged.connect(wrapper.markDirty);
    }

    function save() {
        // Supress saving messages when there is no project.
        if (project && !objEdit.readOnly)
            project.objectCodeText = objEdit.text;
    }
    KDDW.DockingArea {
        id: dockWidgetArea
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: visibilityBar.top
        }
        // Need application-wide unique ID, otherwise opening a new project will confuse the global name resolution algorithm.
        // TODO: Not gauranteed to be unique, but should be good enough for our purposes.
        uniqueName: Math.ceil(Math.random() * 1000000000).toString(16)
        KDDW.DockWidget {
            id: dock_object
            title: "Object Code"
            uniqueName: `ObjectCode-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": true
            }
            Text.ObjTextEditor {
                id: objEdit
                anchors.fill: parent
                readOnly: mode !== "editor"
                // text is only an initial binding, the value diverges from there.
                text: project?.objectCodeText ?? ""
            }
        }
        KDDW.DockWidget {
            id: dock_greencard
            title: "Instructions"
            uniqueName: `Instructions-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": false
            }
            Utils.GreencardView {
                id: greencard
                // property size kddockwidgets_min_size: Qt.size(300, 100)
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                architecture: project?.architecture ?? Architecture.PEP10
                hideStatus: true
                hideMnemonic: true
                dyadicAddressing: true
                //visible: mode === "editor"
            }
        }
        KDDW.DockWidget {
            id: dock_input
            title: "Batch Input"
            uniqueName: `BatchInput-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": true
            }
            IO.Batch {
                id: batchInput
                anchors.fill: parent
                property bool ignoreTextChange: false
                Component.onCompleted: {
                    onTextChanged.connect(() => {
                        if (!ignoreTextChange)
                            project.charIn = text;
                    });
                }
                function setInput(input) {
                    ignoreTextChange = true;
                    batchInput.text = input;
                    ignoreTextChange = false;
                }
            }
        }
        KDDW.DockWidget {
            id: dock_output
            title: "Batch Output"
            uniqueName: `BatchOutput-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            IO.Batch {
                id: batchOutput
                anchors.fill: parent
                text: project?.charOut ?? ""
                readOnly: true
            }
        }
        KDDW.DockWidget {
            id: dock_cpu
            title: "CPU Dump"
            uniqueName: `RegisterDump-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            ColumnLayout {
                anchors.fill: parent
                property size kddockwidgets_min_size: Qt.size(registers.implicitWidth, registers.implicitHeight)
                Cpu.RegisterView {
                    id: registers
                    Layout.fillWidth: false
                    Layout.alignment: Qt.AlignHCenter

                    registers: project?.registers ?? null
                    flags: project?.flags ?? null
                }
            }
        }
        KDDW.DockWidget {
            id: dock_hexdump
            title: "Memory Dump"
            uniqueName: `MemoryDump-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": true
            }
            Loader {
                id: loader
                anchors.fill: parent
                Component.onCompleted: {
                    const props = {
                        "memory": project.memory,
                        "mnemonics": project.mnemonics
                    };
                    // Construction sets current address to 0, which propogates back to project.
                    // Must reject changes in current address until component is fully rendered.
                    con.enabled = false;
                    setSource("qrc:/qt/qml/edu/pepp/memory/hexdump/MemoryDump.qml", props);
                }
                asynchronous: true
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
    ListView {
        id: visibilityBar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 15
        orientation: Qt.Horizontal
        model: [dock_object, dock_greencard, dock_input, dock_output, dock_cpu, dock_hexdump]
        delegate: CheckBox {
            text: modelData.title
            checked: modelData.isOpen
            onClicked: {
                modelData.visibility[wrapper.mode] = checked;
                checked ? modelData.open() : modelData.close();
            }
            Layout.alignment: Qt.AlignBottom
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
