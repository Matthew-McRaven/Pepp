pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qt.labs.qmlmodels
import "move.js" as Move

Item {
    id: root

    property var fromObject: null
    property var toObject: null

    x: Math.min(fromObject.inputXY.x, toObject.outputXY.x )
    y: Math.min(fromObject.inputXY.y, toObject.outputXY.y )
    width: Math.max(1,  Math.abs(fromObject.inputXY.x - toObject.outputXY.x))
    height: Math.max(1, Math.abs(fromObject.inputXY.y - toObject.outputXY.y))

    /*
    anchors.left: fromObject.right
    anchors.top: fromObject.y <= toObject.y ? fromObject.top : toObject.top
    anchors.topMargin: fromObject.y === toObject.y ? 0 : fromObject.height / 2

    anchors.right: toObject.left
    anchors.bottom: fromObject.y >= toObject.y ? fromObject.bottom : toObject.bottom
    anchors.bottomMargin: fromObject.y === toObject.y ? 0 :toObject.height / 2*/

    Rectangle {
        anchors.fill: root
        color: "green"
        visible: root.fromObject !== null && toObject !== null
    }
}
/*TableModel {
        id: mod
        TableModelColumn { display: "left" }
        TableModelColumn { display: "middle" }
        TableModelColumn { display: "right" }

        // Each row is one type of fruit that can be ordered
        rows: [
            {
                // Each property is one cell/column.
                left: "tl",
                middle: "tm",
                right: "tr"
            },
            {
                // Each property is one cell/column.
                left: "ml",
                middle: "mm",
                right: "mr"
            },
            {
                // Each property is one cell/column.
                left: "bl",
                middle: "bm",
                right: "br"
            }
        ]
    }
    TableView {
        id: tv
        anchors.fill: root
        resizableColumns: true
        resizableRows: true
        columnSpacing : 0
        rowSpacing : 0
        //columns: 3
        //rows: 3

        model: mod

        delegate:     Rectangle {
            //anchors.fill: parent
            implicitWidth: root.width / 3
            implicitHeight: root.height / 3
            //Layout.fillWidth: true
            //Layout.fillHeight: true
            border {
                width: 1
                color: "green"
            }
        }
    }
}
*/
/*GridLayout {
    id: line
    columns: 2
    rows: 2
    columnSpacing : 0
    rowSpacing : 0

    Rectangle {
        //anchors.fill: root
        Layout.fillWidth: true
        Layout.fillHeight: true
        border {
            width: 1
            color: "green"
        }
    }
    Rectangle {
        //anchors.fill: root
        Layout.fillWidth: true
        Layout.fillHeight: true
        border {
            width: 1
            color: "green"
        }
    }
    Rectangle {
        //anchors.fill: root
        Layout.fillWidth: true
        Layout.fillHeight: true
        border {
            width: 1
            color: "green"
        }
    }
    Rectangle {
        //anchors.fill: root
        Layout.fillWidth: true
        Layout.fillHeight: true
        border {
            width: 1
            color: "green"
        }
    }
}*/
