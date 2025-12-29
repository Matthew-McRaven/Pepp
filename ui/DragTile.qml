import QtQuick 2.15
import QtQuick.Controls

Item {
    id: root

    property real cellWidth: 64
    required property int modelData

    Rectangle {
        id: cell
        width: root.cellWidth
        height: root.cellWidth / 2

        Drag.active: ma.drag.active
        Drag.hotSpot.x: root.width / 2
        Drag.hotSpot.y: root.height / 2
        color: "red"

        border {
            width: 1
            color: "green"
        }

        Label {
            text: root.modelData
            anchors.centerIn: parent

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            id: ma

            anchors.fill: parent
            drag.target: parent

            onReleased: parent = cell.Drag.target !== null ? cell.Drag.target : cell
        }
    }
}
