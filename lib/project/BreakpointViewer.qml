import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels
import edu.pepp

Item {
    property alias model: bpModel.breakpoints
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
        model: BreakpointTableModel {
            id: bpModel
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
