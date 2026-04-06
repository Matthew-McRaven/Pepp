pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import CircuitDesign

Item {
    id: root
    property DiagramTemplate currentStamp: null
    property alias filter: canvas.filter
    focus: true //  Control with focus receives keyboard events

    Flickable {
        id: flickable
        anchors {
            left: parent.left
            right: vsb.left
            top: parent.top
            bottom: hsb.top
        }
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // Ensure that have non-empty content, even if the canvas is currently empty.
        contentWidth: Math.max(canvas.width, canvas.contentWidth, 1920)
        contentHeight: Math.max(canvas.height, canvas.contentHeight, 1080)
        // A dummy item which gives us something to scroll against
        onContentWidthChanged: console.log("Content width changed", contentWidth)
        onContentHeightChanged: console.log("Content height changed", contentHeight)
        Item {
            width: canvas.contentWidth
            height: canvas.contentHeight
        }
        ScrollBar.vertical: vsb
        ScrollBar.horizontal: hsb

        // We can ignore my previous warning about placing this inside the flickable causing insane memory overhead
        // I avoid overhead by explicitly sizing the canvas to the width of the root item.
        // Due to the hierarchy of mouse presses, the canvas must be inside the flickable for the mouse drag to work properly.
        // Life will be difficult when we re-add zoom at the canvas level---we will need to invert that scale and do some translation.
        GraphicCanvas {
            id: canvas

            //  Data passed to control
            // Tie the canvas's top-left to the flickable's content position
            originX: flickable.contentX
            originY: flickable.contentY
            xScrollbar: hsb.position
            yScrollbar: vsb.position
            template: root.currentStamp
            filter: FilterDiagramListModel.None
            x:0
            y:0
            width: flickable.width
            height: flickable.height
            transform: Translate {
                x: flickable.contentX
                y: flickable.contentY
            }

            clip: true

            //  Context menu for canvas
            ContextMenu.menu: Menu {
                MenuItem {
                    text: "Rotate Left"
                    onTriggered: {
                        var item = canvas.currentDiagram;

                        //  Check to see if there is no current item
                        if (item)
                        canvas.rotateClockwise();
                    }
                }
                MenuItem {
                    text: "Rotate right"
                    onTriggered: {
                        var item = canvas.currentDiagram;

                        //  Check to see if there is no current item
                        if (item)
                        canvas.rotateCounterClockwise();
                    }
                }

            }
        }

    }



    ScrollBar {
        id: vsb
        visible: flickable.contentHeight > flickable.height
        policy: visible? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        width: visible ? implicitWidth : 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
    ScrollBar {
        id: hsb
        visible: flickable.contentWidth > flickable.width
        policy: visible? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }


    Keys.onPressed: event => {
        //  Forward keypress events from QML to canvas
        //  Canvas will return true if keypress was handled
        event.accepted = canvas.keyPress(event.key, event.modifiers);
        //console.log( event.key, event.modifiers);
    }

    //  This is a hack. I'm not able to disable Canvas from taking focus.
    //  This function keeps focus at item level.
    onFocusChanged: focus = true
}   //  Item
