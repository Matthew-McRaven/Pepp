import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels

Item {
    Rectangle {
        id: outline
        color: palette.base
        anchors.fill: parent
        //  Give object code viewer a background box
        border.width: 1
        border.color: palette.mid
    }
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
        anchors.margins: 2
        boundsBehavior: Flickable.StopAtBounds
        resizableColumns: true
        model: TableModel {
            TableModelColumn {
                display: "Address"
            }
            TableModelColumn {
                display: "File"
            }
            TableModelColumn {
                display: "Line"
            }
            TableModelColumn {
                display: "Condition"
            }
            TableModelColumn {
                display: "ConditionValue"
            }

            rows: [{
                    "Address": 0,
                    "File": "User",
                    "Line": 0,
                    "Condition": "",
                    "ConditionValue": ""
                }, {
                    "Address": 67,
                    "File": "User",
                    "Line": 22,
                    "Condition": "$X == 5",
                    "ConditionValue": "false"
                }, {
                    "Address": 0xFFF5,
                    "File": "OS",
                    "Line": 0,
                    "Condition": "$IsDebug",
                    "ConditionValue": "true"
                }]
        }
        delegate: TextInput {
            text: model.display
            selectByMouse: true
            onAccepted: model.display = text
            rightPadding: 10
            leftPadding: 2
            clip: true
        }
    }
}
