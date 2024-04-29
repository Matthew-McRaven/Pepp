import QtQuick
import QtQuick.Controls

Item {
    id: wrapper
    property alias colWidth: cell.implicitWidth
    property alias rowHeight: cell.implicitHeight
    property alias backgroundColor: cell.color
    property alias textColor: display.color
    property alias text: display.text
    property alias textAlign: display.horizontalAlignment
    property alias font: display.font
    property var tooltip: null

    //  Magic fields required by selection model
    required property bool editing
    //required property bool selected
    required property bool current

    //  Make cell placement visible outside control
    implicitHeight: display.implicitHeight
    implicitWidth: display.implicitWidth

    Rectangle {
        id: cell
        anchors.fill: parent

        //  Testing only
        //border.width: 1
        //border.color: "red"

        //  Colors are managed by model
        color: "gray"

        //  Used to create editable cell
        Text {
            id: display
            anchors.fill: cell
            verticalAlignment: Text.AlignVCenter
            renderType: Text.NativeRendering
            visible: !editing
            focus: false
            z: current ? -1 : 1

            ToolTip {
                id: tip
                // If null or empty string, don't display tooltip.
                visible: ((wrapper.tooltip ?? "") !== "") && !editing
                         && ma.hovered
                text: wrapper.tooltip
                delay: 1000
            }

            //  Used to trigger tool tip
            HoverHandler {
                id: ma
                enabled: true
            }
        }
    }
}
