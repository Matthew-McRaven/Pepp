import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

MenuItem {
    id: wrapper
    Label {
        enabled: wrapper.action?.enabled
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        text: wrapper.action.nativeText
        width: tm.width * 20 // 40 spaces wide
        horizontalAlignment: Text.AlignLeft
        rightPadding: tm.width
    }
}
