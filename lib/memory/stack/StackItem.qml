import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root
    property bool isHeader: false
    property int stateChange: 0
    property int address: 0 //  Used by stack records
    property string heading: "" //  Only used by header record
    property string value: ""
    property string name: ""
    property int charWidth: 8

    property color textColor: palette.text
    property font font: Theme.font

    RowLayout {
        anchors.fill: parent
        Rectangle {
            id: addr
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 8 * charWidth
            Layout.maximumWidth: 8 * charWidth
            Label {
                anchors.left: parent.left
                color: root.textColor
                font: root.font
                text: isHeader ? heading : `0x${root.address.toString(
                                     16).padStart(4, '0').toUpperCase()}`
            }
        }
        Rectangle {
            id: val
            border.color: isHeader ? "transparent" : palette.text
            border.width: 1.5
            color: stateChange !== 0 ? palette.highlight : palette.base
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 7 * charWidth
            Layout.maximumWidth: 7 * charWidth
            Label {
                anchors.centerIn: parent
                color: stateChange !== 0 ? palette.highlightedText : root.textColor
                font: root.font
                text: root.value
            }
        }
        Rectangle {
            id: name
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Label {
                anchors.left: parent.left
                color: root.textColor
                font: root.font
                text: root.name
            }
        }
    }
}
