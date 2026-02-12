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
    Item {
        id: viewport
        // Translate moves the children to left... which would clip with other content in the scene even when clip=true
        z: -1
        property real logicalX: flickable.contentX/scene.scale
        property real logicalY: flickable.contentY/scene.scale
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
        clip:true
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
        transformOrigin: Item.TopLeft
        transform: [
            Scale {
                origin.x: viewport.logicalX
                origin.y: viewport.logicalY
                xScale: scene.scale
                yScale: scene.scale
            },
            Translate {
                x: -viewport.logicalX
                y: -viewport.logicalY
            }
        ]

        CursedCPUCanvas {
            id: canvas
            x:0; y:0
            width: canvas.contentWidth; height:canvas.contentHeight
            Component.onCompleted: console.log(width, height)

        }
        Component.onCompleted: console.log(implicitWidth, implicitHeight)
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
        contentWidth: Math.max(viewport.implicitWidth*scene.scale)
        contentHeight: Math.max(viewport.implicitHeight*scene.scale)
        // A dummy item which gives us something to scroll against
        Item {
            id: scene
            width: viewport.implicitWidth*scale
            height: viewport.implicitHeight*scale
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
