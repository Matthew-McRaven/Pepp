import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    // Inputs
    property alias model: listView.model
    property alias contentWidth: listView.contentWidth
    // Output
    property alias currentIndex: listView.currentIndex
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
        focus: true
        focusPolicy: Qt.StrongFocus
        Keys.onUpPressed: listView.currentIndex = Math.max(
                              0, listView.currentIndex - 1)
        Keys.onDownPressed: listView.currentIndex = Math.min(
                                listView.count - 1, listView.currentIndex + 1)
        delegate: BoxedText {
            required property var modelData
            name: modelData.name
        }
    }
}
