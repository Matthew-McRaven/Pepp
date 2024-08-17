import QtQuick

Item {
  id: root

  property int colWidth: 30
  property int rowHeight: 20
  property alias backgroundColor: background.color
  property alias foregroundColor: border.color

  implicitHeight: 20
  implicitWidth: 40

  Rectangle {
    id:background
    anchors.fill: root
    color: "gray"

    //  Testing only
    //border.width: 1
    //border.color: "red"

    Rectangle {
      id: border
      width: 1
      color: "black"
      height: background.height
      anchors.top: background.top
      anchors.centerIn: background
    }
  }
}
