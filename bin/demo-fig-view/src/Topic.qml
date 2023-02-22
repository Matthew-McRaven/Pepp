import QtQuick
import QtQuick.Controls

Item {
  Column {
    id: column
    spacing: 20
    anchors {
      margins: 20
      left: parent.left
      right: parent.right
    }

    //  Topic name
    Label {
      id: title
      font.pixelSize: 22
      width: parent.width
      color: "#ffffff"
      elide: Label.ElideRight
      horizontalAlignment: Qt.AlignLeft
      text: qsTr("Writing Programs")
      background: Rectangle { color: "#ff7d33"; radius: 5 }
    }

  //  Topic contents
    Label {
      id: content
      width: parent.width
      wrapMode: Label.WordWrap
      text: qsTr("Pep/10 is a virtual machine for writing machine language and assemply language programs") //.arg(inPortrait ? qsTr("portrait") : qsTr("landscape"))
    }
  }
}
