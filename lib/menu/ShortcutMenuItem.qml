import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

MenuItem {
    id: wrapper
    implicitWidth: label.implicitWidth + 10 + shortcut.implicitWidth
    rightPadding: 5
    FontMetrics {
        id: fm
        font: wrapper.font
    }

    TextMetrics {
        id: tm
        font: wrapper.font
        text: wrapper.text
    }
    contentItem: Label {
        id: label
        enabled: wrapper.enabled
        text: wrapper.text.replace("&", "")
        color: wrapper.enabled ? palette.text : palette.shadow
        font: wrapper.font
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideNone
        wrapMode: Text.NoWrap
    }

    Label {
        id: shortcut
        enabled: wrapper.enabled
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: fm.averageCharacterWidth
        text: wrapper.action?.nativeText ?? ""
        horizontalAlignment: Text.AlignLeft
        color: wrapper.enabled ? palette.text : palette.shadow
    }
}
