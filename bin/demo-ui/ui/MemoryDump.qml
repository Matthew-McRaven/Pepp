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

  /*Material.theme: Material.Light
  Material.accent: "#888888" //Material.Indigo  //  Cursor
  Material.foreground: Material.theme === Material.Light ?
                         "black" : "#f8f8f8"//  Text color #383838
  Material.background: Material.theme === Material.Light ?
                         "#f8f8f8" : "#383838"//  Background*/
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
    selectionBehavior: TableView.SelectCells //TableView.SelectionDisabled//TableView.SelectCells
    selectionMode: TableView.SingleSelection
    editTriggers: TableView.EditKeyPressed |
                  TableView.SelectedTapped |
                  TableView.DoubleTapped

    model: MemoryByteModel

    property int asciiWidth: MemoryByteModel.BytesPerColumn ?
                             (10 * MemoryByteModel.BytesPerColumn) : 100

    //  For grid column sizes. Currently, columns are not resizeable
    columnWidthProvider: function (column) {
      //console.log(column)
      if(column === MemoryByteModel.Column.LineNo) return colWidth * 2
      else if(column === MemoryByteModel.Column.Border1) return 1
      else if(column === MemoryByteModel.Column.Border2) return 1
      else if(column === MemoryByteModel.Column.Ascii) return asciiWidth
      else return colWidth
    }

    //  Disable horizontal scroll bar
    ScrollBar.horizontal: ScrollBar {
      visible: false
    }

    //  Enable vertical scroll bar-always on
    ScrollBar.vertical: ScrollBar {
      id: vsc
      visible: true
    }

    //  Used for drawing grid
    delegate: Rectangle {
      id: cell

      implicitWidth: colWidth
      implicitHeight: rowHeight

      //  Colors are managed by model
      color: model.backgroundColorRole

      //  Magic fields required by selection model
      required property bool editing
      required property bool selected
      required property bool current

      Keys.onDownPressed: {
        console.log("onDownPressed: row=" + row)
        vsc.position = tableView.bottomRow +
          (tableView.bottomRow - tableView.topRow - 1)
        console.log("onDownPressed: row=" + row)
      }

      Text {
        id: display
        anchors.fill: cell
        horizontalAlignment: model.textAlignRole //Managed in model
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
        visible: !editing

        color: model.textColorRole
        text: model.byteRole
        font: hexFont

        ToolTip {
          //  Suppress tool tip if no message or if cell is being edited
          visible: (model.toolTipRole && !editing) ? ma.containsMouse : false
          delay: 1000
          text: model.toolTipRole
        }

        MouseArea {
          id: ma
          anchors.fill: display
          cursorShape: Qt.IBeamCursor
          hoverEnabled: true

          Timer{
            id:timer
            interval: 200
            onTriggered: singleClick()
          }
          //  Ignore double clicks
          /*onDoubleClicked: (mouse) => {
            console.log("Double click")
            dblClick()
            mouse.accepted = false
          }*/
          onClicked: (mouse) => {
            console.log("Double click" + mouse)
            if(mouse.button === Qt.LeftButton) {
              if(timer.running) {
                dblClick()
                mouse.accepted = false
                timer.stop()
              } else
                timer.restart()
            }
          }
        }
      }

      //  Used to edit a single cell in grid
      TableView.editDelegate: TextInput {
        id: editor
        anchors.fill: display
        padding: 0

        //  Colors are managed in a separate C++ model to allow greater selections
        color: model.textColorRole                    // Text color
        selectedTextColor: model.backgroundColorRole  // Text to be edited
        selectionColor: model.textColorRole           // Current editing field background

        maximumLength: 2
        font: display.font

        horizontalAlignment: display.horizontalAlignment
        verticalAlignment: display.verticalAlignment
        overwriteMode: true
        echoMode: TextInput.Normal
        focus: true

        validator: RegularExpressionValidator { regularExpression: /[0-9,A-F,a-f]{2}/}

        Keys.onEscapePressed: tableView.closeEditor()
        Keys.onLeftPressed: (event) => {
          console.log("editor.onLeftPressed: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)
          if( editor.selectionStart === 0) {
            editor.saveEdits()
            openEditor(0,-1)
          }
          else {
            //  Move left 1 character
            editor.select(editor.selectionStart-1, editor.selectionStart)
            console.log("left: pos,start,end=" + editor.cursorPosition + "," +
              editor.selectionStart + "," + editor.selectionEnd)

            //  Stop event from bubbling up to parent
            event.accept = true
          }
        }
        Keys.onRightPressed: (event) => {
          console.log("editor.onLeftPressed: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)
          if( editor.cursorPosition === 2) {
            editor.saveEdits()
            openEditor(0,1)
          }
          else {
            //  Move right 1 character
            editor.select(editor.selectionStart+1, editor.selectionStart+2)
            console.log("left: pos,start,end=" + editor.cursorPosition + "," +
              editor.selectionStart + "," + editor.selectionEnd)

            //  Stop event from bubbling up to parent
            event.accept = true
          }
        }

        //  Cursor background is white by default. Use TextInput background color
        cursorDelegate: Rectangle {
          visible: editor.cursorVisible
          color: model.backgroundColorRole
          width: editor.cursorRectangle.width

          //  Do not block TextInput with cursor
          z: -1
        }

        //  Performed when control is finished being created
        Component.onCompleted: {
          //console.log("Component.onCompleted: row,column=" + row + "," + column)
          MemoryByteModel.setSelected(MemoryByteModel.index(row, column),MemByteRoles.Editing)

          //  Get all text for editing
          selectAll()
          text = display.text

          //  Make sure first character is visible
          console.log("onCompleted-Before: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)

          //  Highlight first character
          editor.select(0, 1)

          console.log("onCompleted-After: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)
        }

        onTextEdited: {
          //  This function is only activated if valid number or character
          //  is clicked. Arrow keys and other characters are ignored.
          //console.log("onTextChanged: pos,start,end=" + editor.cursorPosition + "," +
          //            editor.selectionStart + "," + editor.selectionEnd)

          //  Highlight next character
          if(editor.cursorPosition === 1 ) {
            editor.select(editor.cursorPosition, editor.cursorPosition + 1)
          }

          //  If on second position after edit, time to go to next filed
          else if(editor.cursorPosition === 2) {
            editor.select(editor.cursorPosition, editor.cursorPosition + 1)
            //  On last character, save and go to next TextField
            editor.saveEdits()
            openEditor(0,1)
          }
        }

        //  Performed when enter is pressed
        TableView.onCommit: {
          console.log("OnCommit: index=" + index + " Text=" + text)

          saveEdits()
          clearEditor()
        }

        Component.onDestruction: {
          console.log("OnCommit: row="+row+" col="+ column)
          MemoryByteModel.clearSelected(MemoryByteModel.index(row, column),
                                        MemByteRoles.Editing)
        }

        function saveEdits() {
          console.log("saveEdits")

          editor.selectAll()
          model.byteRole = text
          display.text = text
        }
      }

      function openEditor(rowOffset, colOffset) {
        console.log("openEditor start: rowOffset="+rowOffset+" columnOffset="+ colOffset)
        console.log("current cell: row="+row+" col="+ column)

        //  If edited field did not move, just return
        if(rowOffset === 0 && colOffset === 0)
          return

        let newRow = row
        let newCol = column

        //  Total rows is 1-based, but row number is zero based
        //  Convert Qml field to last row index
        let lastRowIndex = 8100//tableView.rows - 1

        if( rowOffset < 0 && row === 0) {
          //  Capture moving backwards at beginning of document
          //  Set to last row
          newRow = lastRowIndex
        }
        else if( rowOffset > 0 && lastRowIndex === row) {
          //  Capture moving forward at end of document
          //  Set to first row
          newRow = 0
        }
        else {
          //  By default, just move based on offset
          newRow = newRow + rowOffset
        }

        if( colOffset < 0 &&
            MemoryByteModel.Column.CellStart === column) {
          //  Capture moving backwards at beginning of row
          //  Set to last column
          newCol = MemoryByteModel.Column.CellEnd

          //  If in first row, move to last row.
          newRow = row === 0 ? lastRowIndex : newRow - 1
        }
        else if( colOffset > 0 &&
                MemoryByteModel.Column.CellEnd === column) {
          //  Capture moving forward at end of column
          //  Set to first column in next row
          newCol = MemoryByteModel.Column.CellStart

          //  If in last row, move to first row.
          newRow = row === lastRowIndex ? 0 : newRow + 1
        }
        else {
          //  By default, just move based on offset
          newCol = newCol + colOffset
        }

        //  Check that new cell is visible, if not, change viewport

        /*console.log("bottomRow="+tableView.bottomRow)
        var viewLocation = Math.min(newRow / tableView.rows, 1 - tableView.visibleArea.heightRatio)
        console.log("viewRow, position="+tableView.contentY + "," + viewLocation)
        tableView.contentY = viewLocation
        console.log("viewRow="+tableView.contentY)*/

        var newIndex = MemoryByteModel.index(newRow, newCol)
        //console.log("bottomIndex="+newIndex)
        //tableView.positionViewAtCell(newIndex,TableView.AlignTop, Qt.point(-5, -5))

        tableView.edit(newIndex)
        console.log("openEditor complete: newRow="+newRow+" newCol="+ newCol)
      }

      function clearEditor() {
        console.log("clearEditor")

        //  Reset formatting
        MemoryByteModel.clearSelected(MemoryByteModel.index(row, column),
                                      MemByteRoles.Editing)

        //  Close editor (if open)
        tableView.closeEditor()
      }

      //  Represents a selected field
      function singleClick(){
        /*console.log("Single")

        //  Close editor if changing to new location
        clearEditor()

        //  Always save when leaving previous edited cell.
        let modelIndex = MemoryByteModel.modelCellIndex(index)
        tableView.edit(MemoryByteModel.index(modelIndex.row, modelIndex.column))
      */}

      //  Represent field to be edited
      function dblClick(){
        //  Double click and single click both trigger on doubleclick
        //  Use timing loop to tell these apart. Manually trigger
        //  cell edit if doubleclick
        //console.log("Double")

        console.log("dblClick: Row=" +row + ", Column:" + column)
        //  Row, Column, and Index are hidden variables in edit delegate.
        tableView.edit(MemoryByteModel.index(row, column))
      }
    }
  }
  SelectionRectangle {
          target: tableView
      }
}
