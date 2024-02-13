import QtQuick
import QtQuick.Controls

Rectangle {
  id: root

  //  For testing only. Overridden by parent
  width: 1000
  height: 800
  color: "#e0e0e0"

  property int colWidth: 30
  property int rowHeight: 20
  property int rows: 5

  property int bulletSize: rowHeight * .8
  property alias backgroundColor: root.color
  property color breakpointColor: "red"
  property color highlightColor: "#f8f8f8"
  property alias currentRow: view.currentIndex

  signal clicked

  ListView {
    id: view
    //anchors.fill: parent
    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom;
    width: colWidth;

    clip: true
    focus: false
    interactive: false

    model: rows
    delegate: bulletDelegate

    Component {
      id: bulletDelegate

      Rectangle {
        id: wrapper
        implicitWidth: view.width
        implicitHeight: root.rowHeight

        required property int index

        color: view.currentIndex === index ? root.highlightColor : root.color

        Rectangle {
          id: bullet
          visible: false

          //anchors.left: view.left
          anchors.centerIn: parent
          color: root.breakpointColor
          height: root.bulletSize
          width: root.bulletSize
          radius: root.bulletSize / 2
        }

        MouseArea {
          anchors.fill: parent
          onClicked:  {
            bullet.visible = !bullet.visible;
          //console.info( wrapper.height, wrapper.width);
          }
        }
      }
    }
  }
}
