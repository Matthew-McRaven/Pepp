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
    //property point input: input()
    //property point output: input()

    width: Move.majorX
    height: Move.majorY

    Drag.active: ma.drag.active
    //Drag.hotSpot.x: root.width / 2
    //Drag.hotSpot.y: root.height / 2

    /*function input() {
        var x = 0;
        var y = 0;

        switch (image.rotation) {
        //  Pointing down
        case 90:
            y = root.height;
            x = root.width / 2;
            break;
        //Pointing left
        case 180:
            //  X = 0 already
            y = root.height / 2;
            break;
        //  Pointing up
        case 270:
            x = root.width / 2;
            break;
        //  Pointing left
        default:
            x = root.width;
            y = root.height / 2;
            break;
        }

        return Qt.point(x,y);
    }*/

    Rectangle {
        id: wrapper

        anchors.fill: root
        color: "transparent"
        border.color: ma.drag.active || ma.containsMouse ? "blue" : "transparent"
        border.width: 1
        transformOrigin: Item.Center
        rotation: 0

        //  Output indicator
        Rectangle {
            id: output
            color: "aqua"
            width: 5
            height: 5

            anchors.verticalCenter: image.verticalCenter
            anchors.right: image.right
        }

        //  Input indicator
        Rectangle {
            id: input
            color: "limegreen"
            width: 5
            height: 5

            anchors.verticalCenter: image.verticalCenter
            anchors.left: image.left
        }

        VectorImage {
            id: image
            source: ""

            fillMode: Image.PreserveAspectFit
            //rotation: 0
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
                //  Rotate entire object, including end points
                if (wrapper.rotation >= 270) {
                    wrapper.rotation = 0;
                }
                else
                {
                    wrapper.rotation += 90;
                }

                root.horizontal = (wrapper.rotation % 180) == 0;
            }

            onPositionChanged: event => {
                //  Move object within grid (large axis)
                Move.moveObjectTo(root, root.x + event.x, root.y + event.y);
                //console.log( "x", event.x, "y", event.y, "root x", root.parent.x, "root y", root.parent.y);
            }
        }
    }
}
