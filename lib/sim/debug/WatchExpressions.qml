import QtQuick
import QtQuick.Controls
import edu.pepp

Item {
    NuAppSettings {
        id: settings
    }
    FontMetrics {
        id: fm
    }
    signal updateGUI
    Component.onCompleted: {
        updateGUI.connect(tableModel.onUpdateGUI)
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
        clip: true

        delegate: Item {
            implicitWidth: Math.max(8 * fm.averageCharacterWidth,
                                    textView.implicitWidth + 12)
            implicitHeight: textView.implicitHeight * 1.3
            required property bool editing
            clip: true
            Text {
                id: textView
                anchors.fill: parent
                text: model.display ?? ""
                rightPadding: 10
                leftPadding: 2
                color: model.changed ? settings.extPalette.error.background : palette.windowText
                visible: !editing
                font.italic: model.italicize
            }

            TableView.editDelegate: TextField {
                id: textEdit
                x: textView.x
                y: textView.y
                width: textView.width
                height: textView.height
                text: model.italicize ? "" : display
                TableView.onCommit: display = text
            }
        }
    }
}
