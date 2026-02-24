import QtQuick
import QtQuick.Controls

Item {
    id: root
    required property bool enableBackground
    required property int row
    required property int column
    required property var model
    property alias backgroundColor: background.color
    property alias textColor: rowNum.color
    property alias text: rowNum.text
    property alias textAlign: rowNum.horizontalAlignment
    property alias font: rowNum.font
    // Must clear tooltip text because it is lazily loaded, and reusing it would be bad
    TableView.onReused: tip.text=""
    Rectangle {
        id: background
        visible: root.enableBackground //|| tip.text
        anchors.fill: parent
        // color: "red"
    }
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
        text: ''
    }
    //  Used to trigger tool tip
    HoverHandler {
        id: ma
        enabled: true
        onHoveredChanged: {
            if (hovered && tip.text === "") {
                const index = root.model.index(root.row, root.column);
                const text = root.model.data(index, Qt.ToolTipRole);
                tip.text = text ?? "";
            }
        }
    }
}
