import QtQuick

import CPUPaint
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


    Flickable {
        id: flickable
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // Ensure that have non-empty content, even if the canvas is currently empty.
        contentWidth: Math.max(canvas.width, canvas.contentWidth)
        contentHeight: Math.max(canvas.height, canvas.contentHeight)
        // A dummy item which gives us something to scroll against
        Item {
            width: canvas.contentWidth
            height: canvas.contentHeight
            CursedCPUCanvas {
                id: canvas
                anchors.fill: parent
            }
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
        id: rpanel
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: 325
        color: "red"
    }
}
