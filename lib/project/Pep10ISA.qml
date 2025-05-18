import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/qt/qml/edu/pepp/text/editor" as Text
import "qrc:/qt/qml/edu/pepp/memory/hexdump" as Memory
import "qrc:/qt/qml/edu/pepp/memory/io" as IO
import "qrc:/qt/qml/edu/pepp/cpu" as Cpu
import "qrc:/qt/qml/edu/pepp/utils" as Utils
import edu.pepp 1.0

FocusScope {
    id: wrapper
    required property var project
    required property var actions
    required property string mode
    signal requestModeSwitchTo(string mode)

    function syncEditors() {
        project ? save() : null;
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

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        handle: Item {
            implicitWidth: 4
            Rectangle {
                implicitWidth: 4
                anchors.horizontalCenter: parent.horizontalCenter
                height: parent.height
                // TODO: add color for handle
                color: palette.base
            }
        }
        Text.ObjTextEditor {
            id: objEdit
            readOnly: mode !== "editor"
            // text is only an initial binding, the value diverges from there.
            text: project?.objectCodeText ?? ""
            SplitView.minimumWidth: 100
            SplitView.fillWidth: true
        }
        Utils.GreencardView {
            id: greencard
            architecture: project?.architecture ?? Architecture.PEP10
            hideStatus: true
            hideMnemonic: true
            visible: mode === "editor"
            SplitView.minimumWidth: 200
            SplitView.fillWidth: true
            SplitView.preferredWidth: 600
        }

        IO.Labeled {
            id: batchInput
            SplitView.minimumHeight: batchInput.minimumHeight
            SplitView.preferredHeight: (parent.height - registers.height) / 2
            width: parent.width
            label: "Input"
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
        IO.Labeled {
            id: batchOutput
            SplitView.minimumHeight: batchOutput.minimumHeight
            SplitView.preferredHeight: (parent.height - registers.height) / 2
            width: parent.width
            label: "Output"
            text: project?.charOut ?? ""
        }
        Item {
            SplitView.minimumWidth: 340
            SplitView.fillWidth: true
            visible: mode === "debugger" || mode === "editor"
            Cpu.RegisterView {
                id: registers
                visible: mode == "debugger"
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
                height: visible ? registers.implicitHeight : 0
                registers: project?.registers ?? null
                flags: project?.flags ?? null
            }
            Loader {
                id: loader
                anchors {
                    top: registers.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
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
            console.log("I was called");
            wrapper.requestModeSwitchTo("debugger");
        }
    }
    Connections {
        enabled: wrapper.activeFocus
        target: wrapper.actions.build.execute
        function onTriggered() {
            console.log("I was called");
            wrapper.requestModeSwitchTo("debugger");
        }
    }
}
