pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "move.js" as Move

Rectangle {
    id: root
    property string text: ""
    property alias file: image.source
    property bool horizontal: true

    width: 75 //horizontal ? 75 : 75 //50
    height: 75 //horizontal ? 75 /*50*/ : 75

    Drag.active: ma.drag.active
    Drag.hotSpot.x: root.width / 2
    Drag.hotSpot.y: root.height / 2

    color: "transparent"
    border.color: "blue" //ma.drag.active || ma.containsMouse ? "blue" : "transparent"
    border.width: 1

    Image {
        id: image
        source: ""
        anchors.fill: root
        fillMode: Image.PreserveAspectFit
        transformOrigin: root.Center
        rotation: 0
    }

    MouseArea {
        id: ma
        anchors.fill: root
        drag.target: parent
        hoverEnabled: true

        onClicked: {
            if (image.rotation >= 270)
                image.rotation = 0;
            else
                image.rotation += 90;

            //  Track horizontal versus vertical layout
            root.horizontal = (image.rotation % 180) == 0;
        }
    }
}
