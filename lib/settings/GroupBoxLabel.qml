import QtQuick
import QtQuick.Controls

Item {
    property alias backgroundColor: backColor.color
    property alias textColor: label.color
    property alias text: label.text
    implicitWidth: label.width
    implicitHeight: label.height / 4

    Label {
        id: label
        leftPadding: 9
        background: Rectangle {
            id: backColor
            anchors {
                fill: parent
                leftMargin: label.leftPadding
            }
        }
    }
}
