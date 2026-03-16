pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import CircuitDesign

Item {
    id: root
    property DiagramTemplate currentStamp: null
    property alias dataModel: scrollView.model
    property alias filter: canvas.filter
    focus: true //  Control with focus receives keyboard events

    // Do not place inside the flickable!
    // If placed inside the flickable, the canvas's QImage would be the full size of the content.
    // That (massive) QImage would need to be fully repainted on each update.
    // By placing it outside the flickable andmatching the flickable's size, we ensure that the canvas's
    // underlying QImage is as small as possible
    GraphicCanvas {
        id: canvas

        //  Data passed to control
        // Tie the canvas's top-left to the flickable's content position
        originX: scrollView.contentX
        originY: scrollView.contentY
        xScrollbar: vsb.visible ? vsb.width : 0
        yScrollbar: hsb.visible ? hsb.height : 0
        model: root.dataModel
        template: root.currentStamp
        filter: FilterDiagramListModel.None

        anchors.fill: parent
        anchors.bottomMargin: hsb.visible ? hsb.height : 0
        anchors.rightMargin: vsb.visible ? vsb.width : 0
        clip: true

        //  Context menu for canvas
        ContextMenu.menu: Menu {
            MenuItem {
                text: "Rotate Left"
                onTriggered: {
                    var item = canvas.currentItem;

                    //  Check to see if there is no current item
                    if (item)
                        canvas.rotateClockwise();
                }
            }
            MenuItem {
                text: "Rotate right"
                onTriggered: {
                    var item = canvas.currentItem;

                    //  Check to see if there is no current item
                    if (item)
                        canvas.rotateCounterClockwise();
                }
            }
        }
    }

    TableView {
        id: scrollView

        anchors.fill: parent

        z: -1
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // Ensure that have non-empty content, even if the canvas is currently empty.
        contentWidth: canvas.contentWidth
        contentHeight: canvas.contentHeight

        ScrollBar.vertical: ScrollBar {
            id: vsb
            policy: ScrollBar.AsNeeded
        }
        ScrollBar.horizontal: ScrollBar {
            id: hsb
            policy: ScrollBar.AsNeeded
        }
    }   //  TableView

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
