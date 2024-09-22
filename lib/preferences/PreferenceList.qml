import QtQuick
import QtQuick.Controls

//  Represents row in listview
ListView {
    id: root
    clip: true
    currentIndex: 0
    implicitHeight: contentHeight

    delegate: Component {
        Item {
            id: wrapper
            // hack to get LHS of border to be in visible area.
            width: ListView.view.width
            height: info.height
            property var isCurrentItem: ListView.isCurrentItem

            Rectangle {
                color: model.currentList.background
                border.color: wrapper.isCurrentItem ? Theme.accent.background : "transparent"
                border.width: 1
                anchors.fill: parent
                anchors.leftMargin: border.width
            }
            Text {
                id: info
                Component.onCompleted: {
                    const v = Math.max(root.implicitWidth,
                                       info.width + leftPadding + rightPadding)
                    root.implicitWidth = Qt.binding(() => v)
                }
                text: model.currentCategory
                color: model.currentList.foreground
                font: model.currentList.font
                leftPadding: 4
                rightPadding: 2
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.currentIndex = index
                    model.currentPreference = model.currentList
                }
            }
        }
    }
}
