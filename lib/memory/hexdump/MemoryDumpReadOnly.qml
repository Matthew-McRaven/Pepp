import QtQuick
import QtQuick.Controls

Rectangle {
    id: background
    property int colWidth: 30
    property int rowHeight: 20
    property alias backgroundColor: background.color
    property alias textColor: rowNum.color
    property alias text: rowNum.text
    property alias textAlign: rowNum.horizontalAlignment
    property alias font: rowNum.font
    property alias tooltip: tip.text

    implicitWidth: colWidth
    implicitHeight: rowHeight

    Label {
        id: rowNum

        anchors.fill: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        color: palette.text
        text: ""
    }
    ToolTip {
        id: tip
        // If null or empty string, don't display tooltip.
        enabled: ((text ?? "") !== "")
        visible: enabled && ma.hovered
        delay: 1000
    }

    //  Used to trigger tool tip
    HoverHandler {
         id: ma
         enabled: true
    }
}
