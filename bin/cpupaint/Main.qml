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
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
        transform: [
            Scale {
                origin.x: viewport.logicalX
                origin.y: viewport.logicalX
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
        contentWidth: viewport.implicitWidth*scene.scale
        contentHeight: viewport.implicitHeight*scene.scale
        // A dummy item which gives us something to scroll against
        Item {
            id: scene
            width: viewport.implicitWidth
            height: viewport.implicitHeight
        }

        function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)) }
        WheelHandler {
            id: wheelZoom
            target: flickable
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedModifiers: Qt.AltModifier
            onWheel: function(e) {
                e.accepted = true

                const s  = scene.scale
                const k  = (e.angleDelta.y > 0) ? 1.1 : 1.0/1.1
                // At zooms higher than 3, we tend to "focus" on the bottom right corner.
                // There is a mistmatch (bug) between logical and pixel coordinates somewhere,
                // and clamping the zoom minimizes the effect.
                // The clamp also ensures a sane zoom range -- no one needs 10x zoom.
                const s2 = flickable.clamp(s * k, 0.5, 3.0)
                if (s2 === s) return

                const lx = viewport.logicalX + e.x / s       // logical point under cursor
                const ly = viewport.logicalY + e.y / s
                let newLogicalX = lx - e.x / s2
                let newLogicalY = ly - e.y / s2

                // Clamp logical X/Y to be in bounds
                const maxX = Math.max(0, viewport.implicitWidth  - flickable.width  / s2)
                const maxY = Math.max(0, viewport.implicitHeight - flickable.height / s2)
                newLogicalX = flickable.clamp(newLogicalX, 0, maxX)
                newLogicalY = flickable.clamp(newLogicalY, 0, maxY)
                // Must not update scene.scale until all flickable properties have been read.
                scene.scale = s2
                // Update logicalX/Y indirect via binding to contentX/Y
                flickable.contentX = newLogicalX * s2
                flickable.contentY = newLogicalY * s2

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
