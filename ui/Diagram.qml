pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage
import "move.js" as Move

Item {
    id: root
    property string text: ""
    property alias file: image.source
    property bool horizontal: true

    width: Move.majorX
    height: Move.majorY

    Drag.active: ma.drag.active
    //Drag.hotSpot.x: root.width / 2
    //Drag.hotSpot.y: root.height / 2

    Rectangle {
        id: wrapper

        anchors.fill: root
        color: "transparent"
        border.color: ma.drag.active || ma.containsMouse ? "blue" : "transparent"
        border.width: 1

        VectorImage {
            id: image
            source: ""
            //anchors.fill: wrapper
            fillMode: Image.PreserveAspectFit
            rotation: 0
            opacity: ma.drag.active ? .25 : 1

            preferredRendererType: VectorImage.CurveRenderer
            anchors.centerIn: wrapper
            width: wrapper.width
            height: wrapper.width / 2
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

            onPositionChanged: event => {
                //  Move object within grid (large axis)
                Move.moveObjectTo(root, root.x + event.x, root.y + event.y);
                //Move.moveObjectTo(root.parent, root.parent.x + event.x, root.parent.y + event.y);
                //console.log( "x", event.x, "y", event.y, "root x", root.parent.x, "root y", root.parent.y);
            }
        }
    }
}
