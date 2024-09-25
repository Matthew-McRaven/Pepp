import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

MenuItem {
    id: wrapper
    property var color: palette.text
    Label {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        text: wrapper.action.nativeText
        width: tm.width * 21 // 42 spaces wide, enough for Ctrl+Shift+L
        horizontalAlignment: Text.AlignLeft
        rightPadding: tm.width
        color: wrapper.color
    }
}
