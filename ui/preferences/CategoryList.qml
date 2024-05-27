import QtQuick
import QtQuick.Controls

//  Represents row in listview
Rectangle {
  id: root

  required property var model
  color: palette.window // Theme.container.background
  border.color: palette.windowText  //  Theme.container.foreground
  border.width: 1

   ListView {
    id: listView
    model: root.model.categoryList
    anchors.fill: root
    anchors.margins: 1

    delegate: Rectangle {
      id: wrapper

      //  Assigned by model. Must be declared.
      required property string modelData;
      required property int index;

      width: listView.width
      height: info.height
      color: ListView.isCurrentItem ?
              palette.highlight : "transparent"
              //Theme.container.background
               //  "darkslateblue" : "white"
      Text {
        id: info
        text: modelData //listView.model.name
        color: wrapper.ListView.isCurrentItem ?
                palette.highlightedText : palette.windowText
                /*Theme.secondary.foreground :
                Theme.container.foreground*/
                 //  "white" : "black"
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
