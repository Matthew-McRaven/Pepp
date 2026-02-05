pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import CircuitDesign

Item {
    id: root
    property var currentStamp: null
    //property var currentIndex: tableView.selectionModel.currentIndex //selectedItem()
    property alias dataModel: scrollView.model

    // Do not place inside the flickable!
    // If placed inside the flickable, the canvas's QImage would be the full size of the content.
    // That (massive) QImage would need to be fully repainted on each update.
    // By placing it outside the flickable andmatching the flickable's size, we ensure that the canvas's
    // underlying QImage is as small as possible
    GraphicCanvas {
        id: canvas

        // Tie the canvas's top-left to the flickable's content position
        originX: scrollView.contentX
        originY: scrollView.contentY
        contentHeight: 1000 //root.dataModel.rows * 10
        contentWidth: 1000 //root.dataModel.columns * 10

        anchors.fill: parent
    }

    TableView {
        id: scrollView

        z:-1
        anchors.fill: parent
        clip:true
        boundsBehavior: Flickable.StopAtBounds
        // Ensure that have non-empty content, even if the canvas is currently empty.
        contentWidth: Math.max(canvas.width, canvas.contentWidth)
        contentHeight: Math.max(canvas.height, canvas.contentHeight)
        // A dummy item which gives us something to scroll against
        delegate: GridLines {
            implicitWidth: 100
            implicitHeight: 100
        }
        /*Item {
            width: canvas.contentWidth
            height: canvas.contentHeight
        }*/

        ScrollBar.vertical: ScrollBar {
             policy: ScrollBar.AlwaysOn
         }
         ScrollBar.horizontal: ScrollBar {
             policy: ScrollBar.AsNeeded
        }
    }

    /*ScrollView {
        id: scrollView
        anchors.fill: parent
        clip:true

        // Ensure that have non-empty content, even if the canvas is currently empty.
        contentWidth: Math.max(canvas.width, canvas.contentWidth)
        contentHeight: Math.max(canvas.height, canvas.contentHeight)

        contentItem: Item {
            width: canvas.contentWidth
            height: canvas.contentHeight
        }
    }*/
/*
        TableView {
            id: tableView

            clip: true
            columnSpacing: 0
            rowSpacing: 0
            alternatingRows: false
            reuseItems: true
            boundsBehavior: Flickable.StopAtBounds
            selectionBehavior: TableView.SelectCells // TableView.SingleSelection
            selectionMode: TableView.ExtendedSelection
            selectionModel: selectionModel

            delegate: GridLines {
                current: current
                implicitWidth: 100
                implicitHeight: 100
            }

        }   //  TableView
    }*/   //  ScrollView

    /*ItemSelectionModel {
        id: selectionModel
    }

    SelectionRectangle {
        id: selectionRectangle
        target: tableView
        selectionMode: SelectionRectangle.Auto
    }*/
}   //  Item
