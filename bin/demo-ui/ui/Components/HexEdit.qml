import QtQuick
import QtQuick.Controls

Rectangle {
  id: wrapper

  property int value: 17
  property bool showPrefix: true  //  Show 0x
  property bool hex16: true       //  Show ffff == true or ff == false

  Label {
    id: hex

    anchors.fill: parent
    text: qsTr("%1%2")
            .arg(showPrefix ? "0x" : "")
            .arg((value % (hex16 ? 0xffff : 0xff)).toString(16).toUpperCase().padStart(hex16 ? 4 : 2,'0'))
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }
}
