pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "move.js" as Move

Item {
    id: root
    property string text: ""
    property alias file: image.source
    property bool horizontal: true

    width: Move.majorX  //75
    height: Move.majorY //75

    Drag.active: ma.drag.active
    Drag.hotSpot.x: root.width / 2
    Drag.hotSpot.y: root.height / 2

    Rectangle {
        id: wrapper

        anchors.fill: root
        color: "transparent"
        border.color: ma.drag.active || ma.containsMouse ? "blue" : "transparent"
        border.width: 1

        Image {
            id: image
            source: ""
            anchors.fill: wrapper
            fillMode: Image.PreserveAspectFit
            transformOrigin: root.Center
            rotation: 0
        }

        MouseArea {
            id: ma
            anchors.fill: wrapper
            drag.target: root
            hoverEnabled: true
            drag.minimumX: 0
            //drag.maximumX: root.x - wrapper.width
            drag.minimumY: 0
            //drag.maximumY: root.height - wrapper.height

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
}
