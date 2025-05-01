import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/edu/pepp/text/editor" as Text
import "qrc:/edu/pepp/memory/hexdump" as Memory
import "qrc:/edu/pepp/memory/io" as IO
import "qrc:/edu/pepp/cpu" as Cpu
import edu.pepp 1.0

Item {
    id: wrapper
    required property var project
    required property var actions
    required property string mode
    signal requestModeSwitchTo(string mode)
    function requestModeSwitchToDebugger() {
        wrapper.requestModeSwitchTo("debugger")
    }
    function syncEditors() {
        save()
    }
    function preAssemble() {
        if (project === null)
            return
        save()
    }
    Component.onCompleted: {
        // Must connect and disconnect manually, otherwise project may be changed underneath us, and "save" targets wrong project.
        // Do not need to update on mode change, since mode change implies loss of focus of objEdit.
        objEdit.editingFinished.connect(save)
        // Can't modify our mode directly because it would break binding with parent.
        // i.e., we can't be notified if editor is entered ever again.
        wrapper.actions.debug.start.triggered.connect(
                    wrapper.requestModeSwitchToDebugger)
        wrapper.actions.build.execute.triggered.connect(
                    wrapper.requestModeSwitchToDebugger)
    }
    // Will be called before project is changed on unload, so we can disconnect save-triggering signals.
    Component.onDestruction: {
        objEdit.editingFinished.disconnect(save)
        wrapper.actions.debug.start.triggered.disconnect(
                    wrapper.requestModeSwitchToDebugger)
        wrapper.actions.build.execute.triggered.disconnect(
                    wrapper.requestModeSwitchToDebugger)
    }

    function save() {
        // Supress saving messages when there is no project.
        if (project === null)
            return
        else if (!objEdit.readOnly)
            project.objectCodeText = objEdit.text
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
        SplitView {
            visible: mode === "debugger"
            SplitView.minimumWidth: Math.max(registers.implicitWidth,
                                             batchInput.implicitWidth,
                                             batchOutput.implicitWidth) + 20
            orientation: Qt.Vertical
            Cpu.RegisterView {
                id: registers
                SplitView.minimumHeight: registers.implicitHeight + 20
                SplitView.maximumHeight: registers.implicitHeight + 20
                registers: project?.registers ?? null
                flags: project?.flags ?? null
            }
            IO.Labeled {
                SplitView.minimumHeight: batchInput.minimumHeight
                SplitView.preferredHeight: (parent.height - registers.height) / 2
                id: batchInput
                width: parent.width
                label: "Input"
                property bool ignoreTextChange: false
                Component.onCompleted: {
                    onTextChanged.connect(() => {
                                              if (!ignoreTextChange)
                                              project.charIn = text
                                          })
                }
                function setInput(input) {
                    ignoreTextChange = true
                    batchInput.text = input
                    ignoreTextChange = false
                }
            }
            IO.Labeled {
                SplitView.minimumHeight: batchOutput.minimumHeight
                SplitView.preferredHeight: (parent.height - registers.height) / 2
                id: batchOutput
                width: parent.width
                label: "Output"
                text: project?.charOut ?? ""
            }
        }
        Loader {
            id: loader
            Component.onCompleted: {
                const props = {
                    "memory": project.memory,
                    "mnemonics": project.mnemonics
                }
                // Construction sets current address to 0, which propogates back to project.
                // Must reject changes in current address until component is fully rendered.
                con.enabled = false
                setSource("qrc:/edu/pepp/memory/hexdump/MemoryDump.qml", props)
            }
            visible: mode === "debugger"
            asynchronous: true
            SplitView.minimumWidth: 340
            onLoaded: {
                loader.item.scrollToAddress(project.currentAddress)
                con.enabled = true
            }
        }
    }
    Connections {
        id: con
        enabled: false
        target: loader.item
        function onCurrentAddressChanged() {
            project.currentAddress = loader.item.currentAddress
        }
    }
}
