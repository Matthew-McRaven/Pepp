import QtQuick

Rectangle {
    color: "orange"
    property var architecture
    property var abstraction

    // Make sure the drawer is always at least as wide as the text
    // There was an issue in WASM where the titles clipper the center area
    TextMetrics {
        id: textMetrics
        text: "Computer Systems, 200th edition"
    }

    TreeView {
        id: treeView
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: textMetrics.width
    }
    Flickable {
        id: contentFlickable
        anchors {
            left: treeView.right
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        Rectangle {
            anchors.fill: parent
            color: "yellow"
        }
        Loader {
            id: contentLoader
            anchors.fill: parent
            // contentWidth: contentItem.width
            // contentHeight: contentItem.height
        }
    }
}
