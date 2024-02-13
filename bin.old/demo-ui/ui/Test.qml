import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import "./Components" as Ui
import edu.pepperdine 1.0

Rectangle {
  id: root
  anchors.fill: parent
  //width: 1000
  //height: 800

  Material.theme: Material.Light
  Material.accent: "#888888" //Material.Indigo  //  Cursor
  Material.foreground: Material.theme === Material.Light ?
                         "black" : "#f8f8f8"//  Text color #383838
  Material.background: Material.theme === Material.Light ?
                         "#f8f8f8" : "#383838"//  Background
  property font asciiFont:
      Qt.font({family: 'Courier',
                weight: Font.Normal,
                italic: false,
                bold: false,
                pointSize: 10})
  property font hexFont:
      Qt.font({family: 'Courier',
                weight: Font.Normal,
                italic: false,
                bold: false,
                capitalization: Font.AllUppercase,
                pointSize: 10})
  property color highlight: Material.theme === Material.Light ?
                              Material.background.darker(1.5) :
                              Material.background.lighter(1.5) //"#888888"

  property int colWidth: 20
  property int rowHeight: 20

  TableView {
    id: tableView
    anchors.left: root.left
    anchors.top: root.top
    anchors.right: root.right
    anchors.bottom: root.bottom

    rowSpacing: 0
    columnSpacing: 0
    boundsBehavior: Flickable.StopAtBounds
    clip: true

    //  Selection information
    selectionBehavior: TableView.SelectCells
    selectionMode: TableView.SingleSelection
    editTriggers: TableView.EditKeyPressed |
                  TableView.SelectedTapped |
                  TableView.DoubleTapped

    model: MemoryByteModel

    //  Used to determine column sizing of table. Last column is 100
    columnWidthProvider: function (column) { if(column === 0) return colWidth * 2
                                             else if(column > 8) return 80
                                             else return colWidth }

    //  Disable horizontal scroll bar
    ScrollBar.horizontal: ScrollBar {
      visible: false
    }

    //  Enable vertical scroll bar-always on
    ScrollBar.vertical: ScrollBar {
      visible: true
    }

    //  Used for drawing grid
    delegate: Rectangle {
      id: cell
      implicitWidth: colWidth
      implicitHeight: rowHeight

      //  Colors are managed by model
      color: model.backgroundColorRole
      //border.width: cell.isSelected ? 1 : 0
      //border.color: cell.isSelected ? "red" : root.color

      //  Magic fields required by selection model - Didn't work
      //required property bool selected
      //required property bool current

      //  Selection criteria tied to model
      property bool isSelected: false

      Text {
        id: display
        anchors.fill: cell
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering

        color: model.textColorRole //"black"
        text: model.byteRole
        font: hexFont

        onActiveFocusChanged: {
        //onEditingFinished: {
          console.log( index + (display.activeFocus ? " Cell Getting" : " Cell Losing") + " focus.")
        }

        MouseArea {
          anchors.fill: display
          //  Ignore double clicks
          onDoubleClicked: (mouse) => {
            console.log("Double click")
            dblClick()
            mouse.accepted = false
          }
          onClicked: {
            console.log("Display Single click")
            singleClick()
          }

        }
      }

      //  Used to edit a single cell in grid
      TableView.editDelegate: TextInput {
        id: editor
        anchors.fill: display
        padding: 0

        color: model.textColorRole // Text color
        selectedTextColor: "white"//color  //model.textColorRole
        selectionColor: Qt.lighter(model.backgroundColorRole,1.5) // Current editing field

        maximumLength: 2

        font: display.font

        Keys.onEscapePressed: tableView.closeEditor()

        horizontalAlignment: display.horizontalAlignment
        verticalAlignment: display.verticalAlignment
        overwriteMode: true
        cursorVisible: false
        cursorPosition: 0
        focus: true

        validator: RegularExpressionValidator { regularExpression: /[0-9,A-F,a-f]{2}/}

        //  Performed when control is finished being created
        Component.onCompleted: {
          let col = Math.floor(index / 8192)
          let row = index % 8192
          //console.log(row + "," + col)
          MemoryByteModel.setSelected(MemoryByteModel.index(row,col),MemByteRoles.Editing)

          //  Get all text for editing
          selectAll()
          text = display.text

          //  Higlight firce item
          editor.select(0,1)

          //  Make sure first character is visible
          cursorPosition: 0
          cursorVisible: false
        }

        //  Cursor background is white by default. Use TextInput background color
        cursorDelegate: Rectangle {
          visible: editor.cursorVisible
          color: model.backgroundColorRole
          width: editor.cursorRectangle.width

          //  Do not block TextInput with cursor
          z: -1
        }

        //  Performed when enter is pressed
        TableView.onCommit: {
          console.log("index=" + index + " Text=" + text)
          editor.selectAll()
          model.byteRole = text

          let col = Math.floor(index / 8192)
          let row = index % 8192
          MemoryByteModel.clearSelected(MemoryByteModel.index(row,col),MemByteRoles.Editing)
        }

        //  Force editor closed if focus changes before saving
        onActiveFocusChanged: {
        //onEditingFinished: {
          console.log( index + (editor.activeFocus ? " Edit Getting" : " Edit Losing") + " focus.")
          /*if(!editor.activeFocus) {
            //  Losing focus

            clearEditor()
          }*/
        }

        Component.onDestruction: {
          let col = Math.floor(index / 8192)
          let row = index % 8192
          MemoryByteModel.clearSelected(MemoryByteModel.index(row,col),MemByteRoles.Editing)
        }

        //  Background color when editing
        Rectangle {
          anchors.fill: editor
          color: model.backgroundColorRole
          z:-1
        }
      }

      function clearEditor() {
        console.log("clearEditor")
        let col = Math.floor(index / 8192)
        let row = index % 8192
        MemoryByteModel.clearSelected(MemoryByteModel.index(row,col),MemByteRoles.Editing)
        tableView.closeEditor()
      }

      //  Represents a selected field
      function singleClick(){
        console.log("Single")
        let col = Math.floor(index / 8192)
        let row = index % 8192
        //console.log(row + "," + col)
        console.log(index)

        //  Close editor if changing to new location
        clearEditor()

        cell.isSelected = MemoryByteModel.setSelected(MemoryByteModel.index(row,col))
        console.log(cell.isSelected)
      }

      //  Represent field to be edited
      function dblClick(){
        //  Double click and single click both trigger on doubleclick
        //  Use timing loop to tell these apart. Manually trigger
        //  cell edit if doubleclick
        console.log("Double")
        let col = Math.floor(index / 8192)
        let row = index % 8192

        //  Close editor if changing to new location
        clearEditor()

        tableView.edit(MemoryByteModel.index(row,col))
      }

      /*MouseArea {
        anchors.fill: cell
        propagateComposedEvents: true

        //  Ignore double clicks
        onDoubleClicked: (mouse) => {
          console.log("Double click")
          //dblClick()
          mouse.accepted = false
        }
        onClicked: {
          console.log("Single click")
          //singleClick()
        }
        //  Double click is 2 clicks within 1/5 of a second
        //  Should be handled by OS, but not exposed in QML
        /*Timer{
          id:timer
          interval: 200
          onTriggered: singleClick()
        }
        onClicked: {
          if(timer.running)
          {
            //dblClick()
            timer.stop()
          }
          else
            timer.restart()
        }
      }*/
    }
  }
}
