import QtQuick
import QtQuick.Controls
import Qt.labs.platform as Platform //  Color picker

Item {
  id: root
  required property color color//: color.color
  signal updatedColor(newColor: color) //  Indicates user changed colors

  Button {
    id: text
    anchors.fill: parent
    highlighted: true

    //  Color gets cast to hex value of number
    text: root.color

    background: Rectangle {
      id: color
      color: root.color
    }

    onClicked: {
      colorDialog.open()
    }
  }

  Platform.ColorDialog {
    id: colorDialog
    currentColor: root.color

    //  Signal parent control that color has changed
    onAccepted: {
      root.updatedColor(colorDialog.color)
    }
  }
}
