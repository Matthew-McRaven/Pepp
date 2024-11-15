import QtQuick
import QtQuick.Shapes

Rectangle {
    id: wrapper
    property double ellipsisSize: 5
    property alias ellipsisHeight: ellipsis.implicitHeight
    property bool hidden: false

    Column {
        id: ellipsis
        anchors.centerIn: parent
        spacing: 2
        Rectangle {
            width: ellipsisSize
            height: ellipsisSize
            color: hidden ? "transparent" : palette.text
            radius: 180
        }
        Rectangle {
            width: ellipsisSize
            height: ellipsisSize
            color: hidden ? "transparent" : palette.text
            radius: 180
        }
        Rectangle {
            width: ellipsisSize
            height: ellipsisSize
            color: hidden ? "transparent" : palette.text
            radius: 180
        }
    }
}
