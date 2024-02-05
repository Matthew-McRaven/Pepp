import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
  //  Set by parent - formatting
  property alias hex16: hex.hex16
  property alias showPrefix: hex.showPrefix

  //  Data elements
  property alias register: label.text
  property alias address: hex.value
  property alias value: value2.text

  property int cellWidth: 60
  //  For testing only. Overridden by parent
  //anchors.fill: parent
  width: 500
  height: 20

  RowLayout {
    id: wrapper
    anchors.fill: parent
    spacing: 5

    Label {
      id: label

      Layout.fillHeight: true
      Layout.fillWidth: true

      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignRight

      text: "Register"
    }

    Rectangle {
      id: display1

      implicitHeight: childrenRect.height ? childrenRect.height : 20
      implicitWidth: cellWidth

      Layout.maximumWidth: cellWidth
      Layout.minimumWidth: cellWidth

      anchors.margins: 1

      HexEdit {
        id: hex

        //  Required fields for child control
        value: address
        showPrefix: true
        hex16: true

        border.color: Material.foreground
        color: Material.background
        border.width: 1

        anchors.fill: parent
      }
    }
    Rectangle {
      id: display2

      implicitHeight: childrenRect.height ? childrenRect.height : 20
      implicitWidth: cellWidth

      Layout.maximumWidth: cellWidth
      Layout.minimumWidth: cellWidth

      anchors.margins: 1

      border.color: Material.foreground
      color: Material.background
      border.width: 1

      Label {
        id: value2

        anchors.fill: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        padding: 0
        color: Material.foreground
        text: "0"
      }
    }
  }
}
