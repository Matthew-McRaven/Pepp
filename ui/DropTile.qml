import QtQuick 2.15

DropArea {
    id: root

    property real cellWidth: 64

    width: root.cellWidth
    height: root.cellWidth / 2

    onDropped: {

    }

    Rectangle {
        id: copy
        anchors.fill: parent

        visible: parent.containsDrag

        border {
            width: 1
            color: parent.containsDrag ? "blue" : "grey"
        }

    }
}
