import QtQuick
import QtQuick.Controls

Item {
  property alias backgroundColor: backColor.color
  property alias textColor: label.color
  property alias text: label.text

  Label {
    id: label
    x: 10
    y: -height/2
    rightPadding: 5
    leftPadding: 5
    background: Rectangle {
      id: backColor
      color: "#ffffff"
    }
    text: "Parent Data"
  }
}
