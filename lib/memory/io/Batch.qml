import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp

ColumnLayout {
    id: root
    NuAppSettings {
        id: settings
    }
    function forceFocusEditor() {
        area.forceActiveFocus();
    }
    property alias text: area.text
    property alias readOnly: area.readOnly
    property real minimumHeight: scrollViewMinHeight
    property real scrollViewMinHeight: 100

    spacing: 0

    ScrollView {
        id: scrollView
        Layout.minimumHeight: root.scrollViewMinHeight
        Layout.margins: 0 //2
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentHeight: Math.max(area.contentHeight, height)
        TextArea {
            id: area
            font: settings.extPalette.baseMono.font
            background: Rectangle {
                color: palette.base
                border.color: palette.shadow
                border.width: 1
            }
        }
    }
}
