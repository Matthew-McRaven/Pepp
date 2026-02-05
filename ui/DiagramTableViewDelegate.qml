pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Control {
    id: control

    required property int row
    required property int column
    required property bool current
    required property bool editing
    required property bool selected
    required property var model

    //  Custom property
    property string color: "white"

    width: 100
    height: 100

    background: Rectangle {
        border.width: control.current ? 2 : 0
        border.color: "blue"
        color: control.editing ? "red" : control.selected ? "yellow" : "white"
        z: -1
    }
    contentItem: MouseArea {
        id: dragArea

        width: 100
        height: 100
        anchors.centerIn: parent

        drag.target: tile
        //drag.minimumX: 10
        //drag.axis: Drag.XandYAxis
        //drag.maximumX: root.x - wrapper.width
        //drag.minimumY: 10
        acceptedButtons: Qt.LeftButton
        //drag.maximumY: root.height - wrapper.height

        onPressed: mouse => {
            console.log("onPressed", mouse.x, mouse.y);
            //tile.Drag.startDrag();
            //console.log("onPressed", mouse);
            //mouse.accepted = false;
        }

        onReleased: mouse => {
            console.log("onRelease", mouse.x, mouse.y);
            console.log("onRelease", tile.Drag.target, tile.Drag.source);
            const bottom = (mouse.x % 100) > 50;
            const right = (mouse.y % 100) > 50;
            tile.parent = tile.Drag.target !== null ? tile.Drag.target : dragArea;
            tile.Drag.source.z = 2;
            console.log("onRelease-before", tile.anchors.topMargin, tile.anchors.leftMargin);
            tile.anchors.topMargin = bottom ? 0 : 50
            tile.anchors.leftMargin = right ? 0 : 50
            console.log("onRelease-after", tile.anchors.topMargin, tile.anchors.leftMargin);
            //control.color = tile.Drag.target !== tile.Drag.source ? "red" : "blue";
            //tile.anchors.verticalCenter = bottom ? parent.bottom : parent.verticalCenter;
            //tile.anchors.horizontalCenter = right ? parent.right : parent.horizontalCenter;
            //tile.anchors.verticalCenter = parent.verticalCenter;
            //tile.anchors.horizontalCenter = parent.horizontalCenter;
        }


        Rectangle {
            id: tile

            width: 75
            height: 75
            anchors {
                //  Reset on drag action below
                verticalCenter: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }

            color: "transparent"//control.color
            opacity: .5
            //border.width: 1
            //border.color: "yellow"

            Drag.active: dragArea.drag.active
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2

            Text {
                anchors.fill: parent
                color: "black"
                font.pixelSize: 48
                text: `${control.column},${control.row}`
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                z: 1
            }
            //! [1]
            states: State {
                when: dragArea.drag.active
                AnchorChanges {
                    target: tile
                    anchors {
                        verticalCenter: undefined
                        horizontalCenter: undefined
                    }
                }
            }
        }
    }

    DropArea {
        width: 100
        height: 100

        //anchors.fill: parent
        //Drag.active: dragArea.drag.active
        //enabled: dragArea.drag.active

        Rectangle {
            anchors.fill: parent
            color: "blue"
            opacity: .5

            // This property keeps the interactive value to restore it after
            // dragging is finished. The reason is that when the interactive
            // mode is true, it steals the events and prevents the drop area
            // from working.
            //property bool restoreInteractiveValue: control.parent.tableView.interactive

            visible: parent.containsDrag
        }
    }
}
