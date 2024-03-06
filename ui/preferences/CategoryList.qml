import QtQuick
import QtQuick.Controls

//  Represents row in listview
Rectangle {
  id: root

  //  Signal other windows that model has changed
  signal updateLayout()

  property alias model: listView.model
  color: "white"
  border.color: "#c0c0c0"
  border.width: 1

  Component {
    id: categoryDelegate
    Rectangle {
      id: wrapper
      width: listView.width;
      height: info.height
      color: ListView.isCurrentItem ? "darkslateblue" : "white"
      Text {
        id: info
        text: model.categories//model.name//model.categories
        color: wrapper.ListView.isCurrentItem ? "white" : "black"
        padding: 2
      }
      MouseArea
      {
        anchors.fill: wrapper
        onClicked: {

          listView.currentIndex = index
          //console.log("CatList.onClick: " & index)
          root.model.category = index
          root.updateLayout()
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

    //  Trigger change in right pane
    //onCurrentItemChanged: {
      //root.model.category = root.model.;
    //}
  }
}
