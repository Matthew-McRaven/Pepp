import QtQuick
import QtQuick.Controls
import Qt.labs.platform as Platform //  Color picker

Item {
  id: root
  required property color color

  //  Indicates user changed colors
  signal updatedColor(newColor: color)

  Button {
    id: wrapper
    anchors.fill: parent
    highlighted: true

    contentItem: Text {
      id: textItem
      //  Color gets cast to hex value of number
      text: root.color
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter

      //  Use Hsv Value to pick highest contrast text
      color: root.color.hsvValue > .5 ? "black" : "white"
    }

    background: Rectangle {
      id: color
      //  Shows current color
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
