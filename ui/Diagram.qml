pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage

import CircuitDesign 1.0
import "move.js" as Move

Item {
    id: root
    property string text: ""
    property string type: ""
    property alias file: image.source
    property bool horizontal: true
    property alias input: input
    property alias output: output
    property point inputXY: inputPt()
    property point outputXY: outputPt()
    //property DiagramProperties props: null

    width: Move.blockWidth
    height: Move.blockHeight

    Drag.active: ma.drag.active
    Drag.hotSpot.x: root.width / 2
    Drag.hotSpot.y: root.height / 2

    function inputPt() {
        var x = root.x;
        var y = root.y;

        switch (wrapper.rotation) {
            //  Pointing down
        case 90:
            x += root.width / 2;
            y += root.height;
            break;
            //Pointing right
        case 180:
            //  X already at top
            y += root.height / 2;
            break;
            //  Pointing up
        case 270:
            x += root.width / 2;
            break;
            //  Pointing left
        default:
            x += root.width;
            y += root.height / 2;
            break;
        }

        return Qt.point(x, y);
    }

    function outputPt() {
        var x = root.x;
        var y = root.y;

        switch (wrapper.rotation) {
            //  Pointing down
        case 90:
            x += root.width / 2;
            break;
            //Pointing right
        case 180:
            x += root.width;
            y += root.height / 2;
            break;
            //  Pointing up
        case 270:
            x += root.width / 2;
            y += root.height;
            break;
            //  Pointing left
        default:
            //  X = 0 already
            y += root.height / 2;
            break;
        }

        return Qt.point(x, y);
    }

    DiagramProperty {
        id: prop
    }

    Rectangle {
        id: wrapper

        anchors.fill: root
        color: "white"
        border.color: ma.drag.active || ma.containsMouse ? "blue" : "transparent"
        border.width: 1
        transformOrigin: Item.Center
        rotation: 0

        function rotateClockwise() {
            //  Rotate entire object, including end points
            if (wrapper.rotation >= 270) {
                wrapper.rotation = 0;
            } else {
                wrapper.rotation += 90;
            }
        }

        function rotateCounterClockwise() {
            //  Rotate entire object, including end points
            if (wrapper.rotation <= 0) {
                wrapper.rotation = 270;
            } else {
                wrapper.rotation -= 90;
            }
        }

        ContextMenu.menu: Menu {
            MenuItem {
                text: "Rotate Left"
                onTriggered: wrapper.rotateClockwise()
            }
            MenuItem {
                text: "Rotate right"
                onTriggered: wrapper.rotateCounterClockwise()
            }
        }

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
            acceptedButtons: Qt.LeftButton
            //drag.maximumY: root.height - wrapper.height

            onClicked: mouse => {

                //  Rotate entire object, including end points
                if (mouse.modifiers & Qt.ShiftModifier) {
                    wrapper.rotateCounterClockwise();
                } else {
                    wrapper.rotateClockwise();
                }

                root.horizontal = (wrapper.rotation % 180) == 0;
            }

            onDoubleClicked: mouse => {
                console.log("doubleclick")
                //root.props.diagram = root;
                mouse.accepted = true;
            }

            onPositionChanged: mouse => {
                //  Only reposition if mouse is pressed
                if (ma.pressedButtons & Qt.LeftButton) {
                    //  Move object within grid (minor axis)
                    const row = Move.xMinorGrid(root.x + mouse.x);
                    const col = Move.yMinorGrid(root.y + mouse.y);
                    root.x = row;
                    root.y = col;
                    //console.log("x", mouse.x, "y", mouse.y, "row", row, "col", col);
                }
            }
        }
    }
}
