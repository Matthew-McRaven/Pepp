import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/ui/text/editor" as Text
import "qrc:/ui/memory/hexdump" as Memory
import edu.pepp

Item {
    id: wrapper
    required property var project
    required property string mode
    Component.onCompleted: {
        //console.log("open", project, "with oc:" + project.objectCodeText)
        // Must connect and disconnect manually, otherwise project may be changed underneath us, and "save" targets wrong project.
        // Do not need to update on mode change, since mode change implies loss of focus of objEdit.
        objEdit.editingFinished.connect(save)
    }
    // Will be called before project is changed on unload, so we can disconnect save-triggering signals.
    Component.onDestruction: {
        objEdit.editingFinished.disconnect(save)
        // console.log("close", project, "with oc:" + project.objectCodeText)
    }

    function save() {
        // Supress saving messages when there is no project.
        if (project === null)
            return
        else if (!objEdit.readOnly)
            project.objectCodeText = objEdit.text
        //console.log("triggering save with object code:" + project.objectCodeText)
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
        }
        Rectangle {
            id: debug
            visible: mode === "debug"
            color: 'green'
            SplitView.minimumWidth: 100
        }
        Loader {
            source: "qrc:/ui/memory/hexdump/MemoryDump.qml"
            visible: mode === "debug"
            asynchronous: true
            SplitView.minimumWidth: 100
        }
    }
}
