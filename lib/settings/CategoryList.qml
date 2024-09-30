import QtQuick
import QtQuick.Controls

import edu.pepp

Rectangle {
    id: root

    // Can't use alias or is not propogated
    property alias model: listView.model
    property alias contentWidth: listView.contentWidth
    // Re-add margins to center ListView items within rectangle
    implicitHeight: listView.contentItem.childrenRect.height
                    + listView.anchors.margins + border.width * 2
    implicitWidth: listView.contentItem.childrenRect.width
                   + listView.anchors.margins * border.width * 2
    color: palette.base
    border {
        color: palette.text
        width: 1
    }

    ListView {
        id: listView
        anchors.fill: parent
        anchors.margins: 2
        implicitHeight: contentItem.childrenRect.height
        implicitWidth: contentItem.childrenRect.width
        clip: true
        delegate: Item {
            id: wrapper
            implicitWidth: childrenRect.width
            implicitHeight: info.height
            required property var modelData
            required property int row
            property bool isCurrentItem: ListView.isCurrentItem

            Rectangle {
                id: fill
                anchors {
                    fill: parent
                    leftMargin: border.width
                    rightMargin: border.width
                }
                color: "transparent"
                border {
                    color: wrapper.isCurrentItem ? palette.highlight : "transparent"
                    width: 1
                }
            }
            Text {
                id: info
                anchors.left: wrapper.left
                text: modelData.name
                color: palette.windowText
                // Prevent text from clipping higlight rectangle
                leftPadding: 4 * fill.border.width
                rightPadding: 4 * fill.border.width
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listView.currentIndex = row
                }
            }
        }
    }
}
