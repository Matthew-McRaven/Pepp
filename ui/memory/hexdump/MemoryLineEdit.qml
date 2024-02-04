import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
  id: root
  //  Data elements
  property alias row: rowNumber.row
  property alias binary: hex.value
  property alias ascii: ascii.text

  property int cellWidth: 20
  //  For testing only. Overridden by parent
  //anchors.fill: parent
  width: 500
  height: 20

  RowLayout {
    id: wrapper
    anchors.fill: parent
    spacing: 5

    RowNumber {
      id: rowNumber

      backgroundColor: Material.background
      textColor: Material.foreground
      highlightColor: "#f8f8f8"
      display: 1 // Force hex display

      //Layout.fillHeight: true
      //Layout.fillWidth: true

      Layout.maximumWidth: cellWidth * 2
      Layout.minimumWidth: cellWidth * 2

      row: 8
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
        value: 0
        showPrefix: false
        hex16: false

        //border.color: Material.foreground
        color: Material.background
        //border.width: 1

        anchors.fill: parent
      }
    }
    Rectangle {
      id: display2

      implicitHeight: childrenRect.height ? childrenRect.height : 20
      implicitWidth: cellWidth * 4

      Layout.maximumWidth: cellWidth * 4
      Layout.minimumWidth: cellWidth * 4

      anchors.margins: 1

      //border.color: Material.foreground
      color: Material.background
      //border.width: 1

      Label {
        id: ascii

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
