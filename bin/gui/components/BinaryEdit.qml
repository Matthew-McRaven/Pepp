import QtQuick
import QtQuick.Controls

Rectangle {
  id: wrapper

  property int value: 0xfffe
  property bool showPrefix: true //  Show 0b
  property bool hex16: false     //  Show 16 bit == true or 8 bit == false

  Label {
    id: bin

    anchors.fill: parent
    text: qsTr("%1%2")
            .arg(showPrefix ? "0b" : "")
            .arg((value % (hex16 ? 0xffff : 0xff)).toString(2).padStart(hex16 ? 16 : 8,'0'))
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }
}
