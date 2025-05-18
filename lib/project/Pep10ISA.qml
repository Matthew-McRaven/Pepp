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
    property bool needsDock: true
    focus: true
    signal requestModeSwitchTo(string mode)

    function syncEditors() {
        project ? save() : null;
    }
    // Call when the height, width have been finalized.
    // Otherwise, we attempt to layout when height/width == 0, and all our requests are ignored.
    function dock() {
        const StartHidden = 1;
        const PreserveCurrent = 2;
        const memcolwidth = registers.implicitWidth;
        const memdumpheight = parent.height - registers.implicitHeight;
        const gcwidth = parent.width * .3;
        const ioheight = 200;
        dockWidgetArea.addDockWidget(dock_object, KDDW.KDDockWidgets.Location_OnLeft, dockWidgetArea, Qt.size(parent.width - gcwidth - memcolwidth, parent.height - ioheight));
        dockWidgetArea.addDockWidget(dock_greencard, KDDW.KDDockWidgets.Location_OnRight, dockWidgetArea, Qt.size(gcwidth, parent.height - ioheight));
        dockWidgetArea.addDockWidget(dock_input, KDDW.KDDockWidgets.Location_OnBottom, dockWidgetArea, Qt.size(parent.width - memcolwidth, ioheight));
        dock_input.addDockWidgetAsTab(dock_output, PreserveCurrent);
        dockWidgetArea.addDockWidget(dock_cpu, KDDW.KDDockWidgets.Location_OnRight, null, Qt.size(memcolwidth, registers.childrenRect.height));
        dockWidgetArea.addDockWidget(dock_hexdump, KDDW.KDDockWidgets.Location_OnBottom, dock_cpu, Qt.size(memcolwidth, parent.height - registers.childrenRect.height));
        wrapper.needsDock = Qt.binding(() => false);
    }

    Component.onCompleted: {
        project.charInChanged.connect(() => batchInput.setInput(project.charIn));
        objEdit.editingFinished.connect(save);
        objEdit.forceActiveFocus();
    }

    function save() {
        // Supress saving messages when there is no project.
        if (project && !objEdit.readOnly)
            project.objectCodeText = objEdit.text;
    }
    KDDW.DockingArea {
        id: dockWidgetArea
        anchors.fill: parent

        uniqueName: "ISA3Layout"

        KDDW.DockWidget {
            id: dock_object
            uniqueName: "Object Code"
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
            uniqueName: "Instructions"
            Utils.GreencardView {
                id: greencard
                // property size kddockwidgets_min_size: Qt.size(300, 100)
                anchors.fill: parent
                architecture: project?.architecture ?? Architecture.PEP10
                hideStatus: true
                hideMnemonic: true
                //visible: mode === "editor"
            }
        }
        KDDW.DockWidget {
            id: dock_input
            uniqueName: "Batch Input"
            IO.Labeled {
                id: batchInput
                anchors.fill: parent
                label: ""
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
            uniqueName: "Batch Output"
            IO.Labeled {
                id: batchOutput
                anchors.fill: parent
                label: ""
                text: project?.charOut ?? ""
            }
        }
        KDDW.DockWidget {
            id: dock_cpu
            uniqueName: "Register Dump"
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
            uniqueName: "Memory Dump"
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

    // Only enable binding from the actions to this project if this project is focused.
    Connections {
        enabled: wrapper.activeFocus
        target: wrapper.actions.debug.start
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
    Connections {
        enabled: wrapper.activeFocus
        target: wrapper.actions.build.execute
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
}
