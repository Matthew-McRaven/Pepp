import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Item {

  property alias label: label.text
  property alias value: data.text
  //required property string value

  //  For testing only. Overridden by parent
  //width: 500
  //height: 400

  Rectangle {
    id: wrapper
    anchors.fill: parent
    color: Material.background
    implicitHeight: 20

    Label {
      id: label

      padding: 2
      anchors.top: wrapper.top
      anchors.left: wrapper.left
      leftPadding: 5

      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignRight

      //  Only for test. Overridden by parent
      text: "N"
    }

    Rectangle {

      anchors.top: wrapper.top
      anchors.left: label.right
      implicitHeight: label.height
      implicitWidth: 15

      border.color: Material.foreground
      color: Material.background

      border.width: 1

      Label {
        id: data

        anchors.fill: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: Material.foreground
        padding: 5

        //  Field only contains 1 or 0
        //  Managed at parent level
        text: value
      }
    }
  }
}
