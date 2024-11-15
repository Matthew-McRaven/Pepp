import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    property alias text: textArea.text
    // Ensure the dialog is large enough
    GridLayout {
        anchors.fill: parent
        Text {
            id: textArea
            Layout.preferredHeight: implicitHeight
            Layout.preferredWidth: implicitWidth
            Layout.fillWidth: true
        }
    }
}
