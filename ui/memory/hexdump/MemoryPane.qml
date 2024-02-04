import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ListView {
  id: root

  clip: true
  focus: true
  interactive: false

  //  For testing only. Overridden by parent
  width: cellWidth * 8
  //height: 800
  //property alias cellWidth: hexDelegate.cellWidth
  property int cellHeight: 20
  property int cellWidth: 20

  //property alias cellWidth: root.cellWidth
  //property alias cellHeight: root.cellHeight

  property font rowfont:
      Qt.font({family: 'Courier',
                weight: Font.Normal,
                italic: false,
                pointSize: 11})
  property color backgroundColor: "#e0e0e0"
  property color textColor: "#888888"
  property color highlightColor: "#f8f8f8"
  property int columns: 8
  property int value: 0

  model: MemoryByteModel
  delegate: memoryDelegate

  Component {
    id: memoryDelegate

    //  Hidden elements of delegate
    //required property int index
    //required property var modelData

    /*HexEdit {
      id: data

      width: cellWidth
      height: cellHeight

      //  Required fields for child control
      value: model.byteRole//model.display
      showPrefix: false
      hex16: false    //  Show as quint8

      color: Material.background
    }*/
    TextEdit {
      id: data

      width: cellWidth
      height: cellHeight

      //  Required fields for child control
      text: model.characterRole
    }
  }
}
