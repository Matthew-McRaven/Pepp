import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    // Inputs
    property alias model: listView.model
    property alias contentWidth: listView.contentWidth
    // Output
    property var currentCategory: model[listView.currentIndex]
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
        clip: true
        delegate: Item {
            id: wrapper
            implicitWidth: childrenRect.width
            implicitHeight: info.height
            required property var modelData
            required property int row
            property bool isCurrentItem: ListView.isCurrentItem
            // Moved before text so that Rectangle will be below Text by default.
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listView.currentIndex = row
                }
                // Dear future progammer, this is a hack. I want all rectangles to have the same width, but since I am autosizing based
                // off of listView.contentItem.childrenRect there are binding loops on implicitWidth.The simplest solution is to
                // make rectangle not be a child of the ListView.contentItem, using nesting such as this.
                // It is **NOT** nested inside Text to avoid having Z-issues.
                Rectangle {
                    id: fill
                    // Specify height with anchors, but use magic math for widths to ensure consistent width between items.
                    anchors {
                        top: parent.top
                        bottom: parent.bottom
                        left: parent.left
                        leftMargin: border.width
                    }
                    // Must manually remove padding for border to prevent clipping on the right.
                    width: wrapper.ListView.view.width - (2 * border.width)
                    color: "transparent"
                    border {
                        color: wrapper.isCurrentItem ? palette.highlight : "transparent"
                        width: 1
                    }
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
        }
    }
}
