import QtQuick

import CircuitDesign
import QtQuick.Controls

Window {
    id: root
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")
    // A dummy left-panel
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
    // Do not place inside the flickable!
    // If placed inside the flickable, the canvas's QImage would be the full size of the content.
    // That (massive) QImage would need to be fully repainted on each update.
    // By placing it outside the flickable andmatching the flickable's size, we ensure that the canvas's
    // underlying QImage is as small as possible
    CursedCanvas {
        id: canvas
        // Tie the canvas's top-left to the flickable's content position
        originX: flickable.contentX
        originY: flickable.contentY
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
    }
    Flickable {
        id: flickable
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
        clip:true
        boundsBehavior: Flickable.StopAtBounds
        // Ensure that have non-empty content, even if the canvas is currently empty.
        contentWidth: Math.max(canvas.width, canvas.contentWidth)
        contentHeight: Math.max(canvas.height, canvas.contentHeight)
        // A dummy item which gives us something to scroll against
        Item {
            width: canvas.contentWidth
            height: canvas.contentHeight
        }

        ScrollBar.vertical: ScrollBar {
             policy: ScrollBar.AlwaysOn
         }
         ScrollBar.horizontal: ScrollBar {
             policy: ScrollBar.AsNeeded
        }
    }
    // A dummy right-panel
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
