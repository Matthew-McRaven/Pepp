import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels          //  For DelegateChooser

import "." as Ui
import edu.pepperdine 1.0

Rectangle {
  id: root
  anchors.fill: parent
  anchors.margins: 5
  color: "lightgray"

  Rectangle {
    id: categories

    anchors.left: parent.left
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.margins: 3

    border.color: "black"
    border.width: 1

    width: 100
    color: "white"
    focus: true

    //  Represents row in listview
    Component {
      id: categoryDelegate
      Rectangle {
        id: wrapper
        width: listView.width;
        height: info.height
        color: ListView.isCurrentItem ? "darkslateblue" : "white"
        Text {
          id: info
          text: categoriesRole
          color: wrapper.ListView.isCurrentItem ? "white" : "black"
        }
        MouseArea
        {
          anchors.fill: wrapper
          onClicked: listView.currentIndex = index
        }
      }
    }

    ListView {
      id: listView
      model: PreferenceModel
      anchors.fill: categories
      anchors.margins: 1

      delegate: categoryDelegate

      //  Trigger change in right pane
      //onCurrentItemChanged:
    }
  }
}
