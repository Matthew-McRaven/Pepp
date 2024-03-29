import QtQuick
import QtQuick.Controls

//  Represents row in listview
Rectangle {
  id: root

  property alias model: listView.model
  color: model.container.background //"white"
  border.color: model.container.foreground //"#c0c0c0"
  border.width: 1

  Component {
    id: categoryDelegate
    Rectangle {
      id: wrapper
      width: listView.width;
      height: info.height
      color: ListView.isCurrentItem ?
              root.model.secondary.background :
              root.model.container.background
               //  "darkslateblue" : "white"
      Text {
        id: info
        text: model.categories
        color: wrapper.ListView.isCurrentItem ?
                root.model.secondary.foreground :
                root.model.primary.foreground
                 //  "white" : "black"
        padding: 2
      }
      MouseArea
      {
        anchors.fill: wrapper
        onClicked: {

          listView.currentIndex = index
          root.model.category = index
        }
      }
    }
  }

  ListView {
    id: listView
    model: PreferenceModel
    anchors.fill: root
    anchors.margins: 1

    delegate: categoryDelegate
  }
}
