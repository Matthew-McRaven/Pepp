import QtQuick

Item {
    id: background
    property alias color: border.color
    Rectangle {
        id: border
        width: 1
        color: palette.windowText
        height: background.height
        anchors.top: background.top
        anchors.centerIn: background
    }
}
