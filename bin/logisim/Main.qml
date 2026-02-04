import QtQuick

import CircuitDesign
import QtQuick.Controls

Window {
    id: root
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")
    Rectangle {
        id: lpanel
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: 150
        color: "orange"
    }
    Flickable {
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
            clip:true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: Math.max(500, parent.width, canvas.contentWidth)
        contentHeight: Math.max(500, parent.height,  canvas.contentHeight)
        CursedCanvas {
            id: canvas
            width: parent.width
            height: parent.height
        }
        ScrollBar.vertical: ScrollBar {
             id: verticalScrollbar
             policy: ScrollBar.AlwaysOn

         }

         ScrollBar.horizontal: ScrollBar {
             id: horizontalScrollbar
             policy: ScrollBar.AsNeeded

        }
    }

    Rectangle {
        id:rpanel
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: 325
        color: "red"
    }


}
