import QtQuick

Item {
    id: wrapper
    implicitWidth: childrenRect.width
    implicitHeight: info.height
    required property string name
    required property int row
    property alias bgColor: fill.color
    property alias fgColor: info.color
    property alias font: info.font
    property bool isCurrentItem: ListView.isCurrentItem
    // Moved before text so that Rectangle will be below Text by default.
    MouseArea {
        anchors.fill: parent
        onClicked: {
            listView.currentIndex = row
            listView.forceActiveFocus()
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
        text: name
        color: palette.windowText
        // Prevent text from clipping higlight rectangle
        leftPadding: 4 * fill.border.width
        rightPadding: 4 * fill.border.width
    }
}
