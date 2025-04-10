import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels

Item {
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.left: tableView.left
        anchors.top: parent.top
        syncView: tableView
        clip: true
        model: tableView.model
        textRole: "display"
        delegate: TextInput {
            text: model.display
        }
    }
    TableView {
        id: tableView
        anchors.left: parent.left
        anchors.top: horizontalHeader.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        boundsBehavior: Flickable.StopAtBounds
        resizableColumns: true
        model: TableModel {
            TableModelColumn {
                display: "Expression"
            }
            TableModelColumn {
                display: "Value"
            }
            TableModelColumn {
                display: "Type"
            }

            rows: [{
                    "Value": 1,
                    "Expression": "$x",
                    "Type": "i16"
                }, {
                    "Value": 4,
                    "Expression": "(u8)($x + 3)",
                    "Type": "u8"
                }, {
                    "Value": 1,
                    "Expression": "$X * $x",
                    "Type": "i16"
                }]
        }
        delegate: TextInput {
            text: model.display
            selectByMouse: true
            onAccepted: model.display = text
            rightPadding: 10
            leftPadding: 2
        }
    }
}
