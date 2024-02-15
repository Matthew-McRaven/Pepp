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


  //  To be replaced with model
  ListModel {
    id: data
    ListElement { name: "Environment"}
    ListElement { name: "Editor"}
  }

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

    ListView {
      model: data
      anchors.fill: categories
      anchors.margins: 1

      delegate: Label {
          id: label
          text: name
          padding: 3
          Rectangle {
            id: background
            anchors.fill: label
            z:-1
          }
      }
    }
  }

  /*StackLayout {
    id: details
    anchors.left: area.right
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
  }*/
}
