import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

Item {
    id: root
    property var architecture
    property var abstraction
    property var selected
    Component.onCompleted: {
        console.log(helpModel.rowCount())
    }
    onSelectedChanged: {
        console.log("selected =", selected)
        const props = helpModel.data(selected, HelpModel.Props)
        const url = helpModel.data(selected, HelpModel.Delegate)

        contentLoader.setSource(url, props)
    }

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
        clip: true
        model: FilteredHelpModel {
            id: helpModel
            model: HelpModel {}
            abstraction: root.abstraction
            architecture: root.architecture
        }

        delegate: TreeViewDelegate {
            id: treeDelegate
            width: treeView.width
            onClicked: {
                root.selected = treeDelegate.treeView.index(row, column)
            }
        }
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
        Loader {
            id: contentLoader
            anchors.fill: parent
            // contentWidth: contentItem.width
            // contentHeight: contentItem.height
        }
    }
}
