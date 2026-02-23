import QtQuick

Rectangle {
    property alias backgroundColor: background.color
    property alias foregroundColor: border.color

    id: background
    color: palette.window

    Rectangle {
        id: border
        width: 1
        color: palette.windowText
        height: background.height
        anchors.top: background.top
        anchors.centerIn: background
    }
}
