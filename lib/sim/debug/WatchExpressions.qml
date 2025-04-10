import QtQuick
import QtQuick.Controls
import edu.pepp

Item {
    FontMetrics {
        id: fm
    }

    property alias watchExpressions: tableModel.watchExpressions
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.top: parent.top
        anchors.left: tableView.left
        anchors.right: parent.right
        syncView: tableView
        clip: true
        textRole: "display"
        delegate: Text {
            text: model.display
            horizontalAlignment: Text.AlignLeft
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
        model: WatchExpressionTableModel {
            id: tableModel
        }

        delegate: Item {
            implicitWidth: Math.max(8 * fm.averageCharacterWidth,
                                    label.implicitWidth + 12)
            implicitHeight: label.implicitHeight

            Text {
                id: label
                anchors.fill: parent
                text: model.display
                rightPadding: 10
                leftPadding: 2
            }
        }
    }
}
