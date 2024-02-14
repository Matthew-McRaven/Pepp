import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import Qt.labs.platform as Platform

Item {

  width: 400
  property int cellWidth: 100
  property int cellHeight: 20

  Row {
    id: wrapper

    spacing: 10

    Label {
      id: label
      text: qsTr("Foreground")

        padding: 0
        leftPadding: 10
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHLeft
    }

    Button {
      id: colorBtn

      property color current: Material.background
      height: cellHeight
      width: cellWidth

      background: Rectangle {
        anchors.fill: parent
        border.color: Material.foreground
        color: colorBtn.down ? Material.foreground : colorBtn.current
        radius: 0
      }

      doubleClicked: Platform.ColorDialog
      {
        id: colorDialog
        currentColor: colorBtn.current
      }
    }

    Button {
      id: resetBtn

      height: cellHeight
      width: cellWidth

      contentItem: Text {
        text: qsTr("Reset")
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: resetBtn.down ? Material.background : Material.foreground
      }

      background: Rectangle {
        anchors.fill: parent
        border.color: Material.foreground
        color: resetBtn.down ? Material.foreground : Material.background
        radius: 0
      }
    }
  }
}
