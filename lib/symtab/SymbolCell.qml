import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
    id: root

    property alias symbolText: symbol.text
    property alias valueText: value.text

    //property alias symbolWidth: symbol.width
    property alias valueWidth: value.width

    property alias backgroundColor: background.color
    property alias textColor: symbol.color
    property alias font: symbol.font

    //implicitWidth: colWidth
    //implicitHeight: rowHeight
    Rectangle {
        id: background
        anchors.fill: root

        Label {
            id: symbol

            anchors.left: parent.left
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            leftPadding: 5

            color: palette.text
            text: ""
        }
        Label {
            id: value

            anchors.left: symbol.right
            anchors.right: parent.right
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight
            rightPadding: 5

            color: root.textColor
            text: ""
            font: root.font
        }
    }
}
