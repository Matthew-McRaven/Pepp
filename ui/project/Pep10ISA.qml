import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/ui/text/editor" as Text
import "qrc:/ui/memory/hexdump" as Memory
import "qrc:/ui/cpu" as Cpu
import edu.pepp

Item {
    id: wrapper
    required property var project
    required property string mode
    Component.onCompleted: {
        // Must connect and disconnect manually, otherwise project may be changed underneath us, and "save" targets wrong project.
        // Do not need to update on mode change, since mode change implies loss of focus of objEdit.
        objEdit.editingFinished.connect(save)
    }
    // Will be called before project is changed on unload, so we can disconnect save-triggering signals.
    Component.onDestruction: {
        objEdit.editingFinished.disconnect(save)
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
                color: 'grey'
            }
        }
        Text.ObjTextEditor {
            id: objEdit
            readOnly: mode !== "edit"
            // text is only an initial binding, the value diverges from there.
            text: project?.objectCodeText ?? ""
            SplitView.minimumWidth: 100
            SplitView.fillWidth: true
        }
        SplitView {
            visible: mode === "debug"
            SplitView.minimumWidth: 200
            orientation: Qt.Vertical
            Cpu.RegisterView {
                id: registers
                SplitView.minimumHeight: 200
                registers: project.registers
                flags: project.flags
            }
            TextArea {
                SplitView.fillHeight: true
            }
        }
        Loader {
            id: loader
            source: "qrc:/ui/memory/hexdump/MemoryDump.qml"
            visible: mode === "debug"
            asynchronous: true
            SplitView.minimumWidth: 340
            onLoaded: {
                if (item !== null) {
                    item.memory = project.memory
                    item.mnemonics = project.mnemonics
                }
            }
        }
    }
}
