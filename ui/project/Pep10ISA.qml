import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/ui/text/editor" as Text

Item {
    required property string mode
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
            readOnly: mode === "debug"
            text: ""
            SplitView.minimumWidth: 100
        }
        Rectangle {
            id: debug
            visible: mode === "debug"
            color: 'green'
            SplitView.minimumWidth: 100
        }
    }
}
