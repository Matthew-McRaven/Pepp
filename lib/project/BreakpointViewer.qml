import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels
import edu.pepp

Item {
    property alias model: bpModel.breakpointModel
    NuAppSettings {
        id: settings
    }
    FontMetrics {
        id: fm
        font: settings.extPalette.baseMono.font
    }
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
        delegate: Text {
            text: model.display
            clip: true
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
        clip: true
        model: BreakpointTableModel {
            id: bpModel
        }

        delegate: Item {
            id: delegate
            implicitWidth: Math.max(8 * fm.averageCharacterWidth,
                                    textView.implicitWidth + 12)
            implicitHeight: Math.max(textView.implicitHeight * 1.3,
                                     fm.height * 1.7)
            Text {
                id: textView
                anchors.fill: parent
                text: model.display
                rightPadding: 10
                leftPadding: 2
                clip: true
                font: fm.font
            }
            // Used for editing conditions
            TableView.editDelegate: TextField {
                id: textEdit
                x: textView.x
                y: textView.y
                width: textView.width
                height: textView.height
                text: model.display
                TableView.onCommit: display = text
                font: settings.extPalette.baseMono.font
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
