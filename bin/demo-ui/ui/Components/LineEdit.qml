import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Rectangle {
  id: root

  //  For testing only. Overridden by parent
  width: 1000
  height: 800
  color: "#e0e0e0"

  //  Properties overriden by parent. Set defaults for testing
  property int colWidth: 30
  property int rowHeight: 20
  property int rows: 16
  property font rowfont:
      Qt.font({family: 'Courier',
                weight: Font.Normal,
                italic: false,
                pointSize: 11})
  property alias backgroundColor: root.color
  property color textColor: "#888888"
  property color highlightColor: "#f8f8f8"

  property alias currentRow: view.currentIndex
  property int display: RowNumbers.LineFormat.Decimal

  ListView {
    id: view
    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom;
    width: colWidth;

    clip: false //true
    focus: false
    interactive: false

    model: rows
    delegate: textDelegate

    Component {
      id: textDelegate

      Rectangle {
        id: wrapper
        implicitWidth: view ? textBox.contentWidth + textBox.leftPadding +
                              textBox.rightPadding : 0//ListView.view.width : 0
        implicitHeight: root.rowHeight

        required property int index

        color: view.currentIndex === index ? root.highlightColor : root.color

        Label {
          id: textBox

          anchors.fill: parent
          horizontalAlignment: Text.AlignLeft
          verticalAlignment: Text.AlignVCenter
          leftPadding: 5; rightPadding: 5

          font.bold: view.currentIndex === index
          color: root.textColor
          text: "Bad.beef"
        }
      }
    }
  }
}
