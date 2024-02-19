import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Rectangle {
  id: root

  enum LineFormat {
    Decimal = 0,
    Hex
  }

  //implicitWidth: view ? ListView.view.width : 0
  //implicitHeight: root.rowHeight

  property int row: 1
  property int colWidth: 30
  property int rowHeight: 20
  property alias backgroundColor: root.color
  property color textColor: "#888888"
  property color highlightColor: "#f8f8f8"
  property int display: RowNumbers.LineFormat.Decimal

  implicitWidth: colWidth
  implicitHeight: rowHeight

  //color: view.currentIndex === index ? root.highlightColor : root.color

  Label {
    id: rowNum

    anchors.fill: parent
    horizontalAlignment: root.display === RowNumber.LineFormat.Decimal ?
      Text.AlignRight :
      Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
    leftPadding: 5; rightPadding: 5

    //font.bold: view.currentIndex === index
    color: root.textColor
    text: root.display === RowNumbers.LineFormat.Decimal ?
      qsTr("%L1").arg(root.row+1 ) :
      qsTr("%1").arg((root.row * 8).toString(16).toUpperCase().padStart(4,'0'))
  }
}

