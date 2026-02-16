pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import CircuitDesign

Item {
    id: root
    property DiagramTemplate currentStamp: null
    //property var currentIndex: tableView.selectionModel.currentIndex //selectedItem()
    property alias dataModel: scrollView.model

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

        anchors.fill: parent
        anchors.bottomMargin: hsb.visible ? hsb.height : 0
        anchors.rightMargin: vsb.visible ? vsb.width : 0
        clip: true
    }

    TableView {
        id: scrollView

        anchors.fill: parent

        z:-1
        clip:true
        boundsBehavior: Flickable.StopAtBounds
        // Ensure that have non-empty content, even if the canvas is currently empty.
        contentWidth: canvas.contentWidth
        contentHeight: canvas.contentHeight
        // A dummy item which gives us something to scroll against
        /*delegate: GridLines {
            implicitWidth: 100
            implicitHeight: 100
        }*/

        ScrollBar.vertical: ScrollBar {
            id: vsb
            policy: ScrollBar.AsNeeded
         }
         ScrollBar.horizontal: ScrollBar {
            id: hsb
            policy: ScrollBar.AsNeeded
        }
    }   //  TableView
}   //  Item
