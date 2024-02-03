import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
  id: root

  property int colWidth: 30
  property int rowHeight: 20
  property alias backgroundColor: background.color
  property alias textColor: rowNum.color
  property alias text: rowNum.text
  property alias textAlign: rowNum.horizontalAlignment
  property alias font: rowNum.font

  implicitWidth: colWidth
  implicitHeight: rowHeight

  Rectangle {
    id: background
    anchors.fill: root
    color: "gray"

    //border.width: 1
    //border.color: "red"

    Label {
      id: rowNum

      anchors.fill: parent
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter

      color: "white"
      text: ""
    }
  }
}

