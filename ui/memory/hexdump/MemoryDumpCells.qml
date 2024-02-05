import QtQuick
import QtQuick.Controls

Item {
  id: root
  property alias colWidth: cell.implicitWidth
  property alias rowHeight: cell.implicitHeight
  property alias backgroundColor: cell.color
  property alias textColor: display.color
  property alias text: display.text
  property alias textAlign: display.horizontalAlignment
  property alias font: display.font

  //  Magic fields required by selection model
  required property bool editing
  //required property bool selected
  required property bool current

  //  Make cell placement visible outside control
  implicitHeight: display.implicitHeight
  implicitWidth: display.implicitWidth

  Rectangle {
    id: cell
    anchors.fill: root

    //  Testing only
    //border.width: 1
    //border.color: "red"

    //  Colors are managed by model
    color: "gray"

    //  Used to create editable cell
    Text {
      id: display
      anchors.fill: cell
      verticalAlignment: Text.AlignVCenter
      renderType: Text.NativeRendering
      visible: !editing
      focus: false
      z: current ? -1 : 1

      ToolTip {
        id: tip
        visible: (!editing && ma.hovered)
        delay: 1000
        text: model.toolTipRole
      }

      //  Used to trigger tool tip
      HoverHandler {
        id: ma
        enabled: true
      }

      Component.onCompleted: {
        //  Limit tool tip to just hex edit cells
        if (column < MemoryByteModel.Column.CellStart ||
            column > MemoryByteModel.Column.CellEnd) {
          tip.visible = false
        }
      }
    }
  }
}
