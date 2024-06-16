import QtQuick
import QtQuick.Controls

//  Represents row in listview
Rectangle {
  id: root

  required property var model

  color: palette.window
  border.color: palette.windowText
  border.width: 1

   ListView {
    id: listView
    model: root.model.categoryList
    anchors.fill: root
    anchors.margins: 2

    delegate: Rectangle {
      id: wrapper

      //  Assigned by model. Must be declared.
      required property string modelData;
      required property int index;

      width: listView.width
      height: info.height
      color: ListView.isCurrentItem ?
              palette.highlight : "transparent"
      Text {
        id: info
        text: modelData
        color: wrapper.ListView.isCurrentItem ?
                palette.highlightedText : palette.windowText
      }
      MouseArea
      {
        anchors.fill: parent
        onClicked: {
          console.log(index)
          listView.currentIndex = index
          root.model.category = index
        }
      }
    }
  }
}
