import QtQuick
import QtQuick.Controls

//  Represents row in listview
Rectangle {
  id: root
  property alias model: listView.model

  Component {
    id: preferenceDelegate
    Rectangle {
      id: wrapper
      width: listView.width;
      height: info.height
      color: model.currentList.background
      border.color: wrapper.ListView.isCurrentItem ? "red" : "transparent"
      border.width: 1

      Text {
        id: info
        text: model.currentCategory
        color: model.currentList.foreground
        font: model.currentList.font
        padding: 2
      }
      MouseArea
      {
        anchors.fill: wrapper
        onClicked: {

          listView.currentIndex = index
          //console.log("CatList.onClick: " + index)
        }
      }
    }
  }

  ListView {
    id: listView
    anchors.fill: root
    anchors.margins: 1
    clip: true
    currentIndex: 0

    delegate: preferenceDelegate
  }
}
