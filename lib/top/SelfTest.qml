import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        syncView: tableView
        clip: true
        textRole: "display"
        delegate: Text {
            text: model.display
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.bold: true
            color: palette.text
        }
    }
    TableView {
        id: tableView
        anchors {
            top: horizontalHeader.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        model: SelfTestModel {}
        delegate: Text {
            text: model.display
            color: palette.text
        }
    }
}
