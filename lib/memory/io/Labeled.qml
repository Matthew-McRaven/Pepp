import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    property alias label: label.text
    property alias text: area.text
    property real minimumHeight: label.height + scrollViewMinHeight
    property real scrollViewMinHeight: 100
    Label {
        id: label
    }
    ScrollView {
        id: scrollView
        Layout.minimumHeight: root.scrollViewMinHeight
        Layout.margins: 2
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentHeight: Math.max(area.contentHeight, height)
        TextArea {
            id: area
            background: Rectangle {
                color: palette.base
                border.color: palette.text
                border.width: 1
            }
        }
    }
}
