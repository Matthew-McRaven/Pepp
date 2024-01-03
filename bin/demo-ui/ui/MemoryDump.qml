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
    selectionBehavior: TableView.SelectionDisabled//TableView.SelectCells
    selectionMode: TableView.SingleSelection
    editTriggers: TableView.EditKeyPressed |
                  TableView.SelectedTapped |
                  TableView.DoubleTapped

    model: MemoryByteModel

    property int asciiWidth: MemoryByteModel.BytesPerColumn ?
                             (10 * MemoryByteModel.BytesPerColumn) : 100

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
      visible: true
    }

    //  Used for drawing grid
    delegate: Rectangle {
      id: cell

      implicitWidth: colWidth
      implicitHeight: rowHeight

      //  Colors are managed by model
      color: model.backgroundColorRole
      border.width: 1
      border.color: cell.isEditMode ? "red" : "transparent"

      //  Magic fields required by selection model - Didn't work
      //required property bool selected
      //required property bool current
      required property bool editing

      //  Selection criteria tied to model
      property bool isSelected: false

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

        /*onActiveFocusChanged: {
        //onEditingFinished: {
          console.log( index + (display.activeFocus ? " Cell Getting" : " Cell Losing") + " focus.")
        }*/

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

        color: model.textColorRole // Text color
        selectedTextColor: model.backgroundColorRole //"white"//color  //model.textColorRole
        selectionColor: model.textColorRole //Qt.lighter(model.backgroundColorRole,1.5) // Current editing field

        maximumLength: 2
        font: display.font

        Keys.onEscapePressed: tableView.closeEditor()

        horizontalAlignment: display.horizontalAlignment
        verticalAlignment: display.verticalAlignment
        overwriteMode: true
        echoMode: TextInput.Normal
        focus: true

        validator: RegularExpressionValidator { regularExpression: /[0-9,A-F,a-f]{2}/}

        //  Background color when editing
        /*Rectangle {
          anchors.fill: editor
          color: model.backgroundColorRole
          z:-1
        }*/

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

          //  Flag that cursor change is programatic so that change cursor does not
          //  Update cursor status
          //cursorPosition = 0
          editor.select(0, 1)

          console.log("onCompleted-After: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)

          //cursorVisible: false
        }

        Keys.onPressed: (event)=> {
          console.log("onTextChanged: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)
          switch(event.key) {
          case Qt.Key_Left:
            console.log("left key");

            // Go to next control
            if(editor.cursorPosition===1) {
              clearEditor()

              //  Move edit focus to left
              tableView.openEditor()
              event.accepted = true
            } else {
              // Move back a character
              editor.select(editor.cursorPosition-1, editor.cursorPosition)
              console.log("left: pos,start,end=" + editor.cursorPosition + "," +
                          editor.selectionStart + "," + editor.selectionEnd)
            }

            break;
          case Qt.Key_Right:
            console.log("right key"); break;
          default:
            console.log("other key"); break;
          }
        }

        onTextEdited: {
          console.log("onTextChanged: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)

          //  Highlight next character
          if(editor.cursorPosition === 1 || editor.cursorPosition === 2)
            editor.select(editor.cursorPosition, editor.cursorPosition + 1)
        }


        //  Performed when enter is pressed
        TableView.onCommit: {
          console.log("OnCommit: index=" + index + " Text=" + text)

          saveEdits()
          clearEditor()
        }

        Component.onDestruction: {
          MemoryByteModel.clearSelected(MemoryByteModel.index(row, column),
                                        MemByteRoles.Editing)
        }

        function updateCursor() {
          return
          /*console.log("updateCursor: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)
          //  We move the cursor to always highlight a full character.
          //  We want to track when a cursor moves so we can highlight the
          //  next character. However, this triggers this function recursively.
          //  Flag when cursor move was manual.
          if(moving) {
            console.log("In edit")
            return
          }

          //  Did user hit backward at start of field, if so, go to previous field
          if( editor.cursorPosition === 0 && !forward) {
            console.log("goto previous field")
            saveEdits()
            clearEditor()

            //  Move edit focus
            tableView.edit(MemoryByteModel.index(row, column-1))
          }
          else if( editor.cursorPosition === 2 && forward) {
            console.log("goto next field")
            tableView.edit(MemoryByteModel.index(row, column+1))
          }
          else { //if( editor.cursorPosition >= 0 && editor.cursorPosition < 2) {

            console.log("Editing")

            moving = true
            editor.select(editor.cursorPosition,editor.cursorPosition +1)
            moving =false

            //editor.cursorVisible = true
            //  Make sure first character is visible
            //cursorVisible: false
          }*/
        }

        function saveEdits() {
          console.log("saveEdits")

          editor.selectAll()
          model.byteRole = text
          display.text = text
        }

      }

      function openEditor() {
        console.log("openEditor")

        //  Open editor. Free function since Qml will only allow nesting up to
        //  3 levels. After 3 levels of nesting, calls in nested functions/objects
        //  cannot see top level TableView
        let rowOffset = 0
        let colOffset = 0
        let newRow = row
        let newCol = column

        if( rowOffset < 0 && row === 0) {
          //  Capture moving backwards at beginning
        }
        else {
          newRow = newRow + rowOffset
        }

        if( colOffset < 0 && column === 0) {
          //  Capture moving backwards at beginning
        }
        else {
          newCol = newCol + colOffset
        }

        //tableView.edit(MemoryByteModel.index(newRow, newCol))
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
        //console.log(index)

        //  Close editor if changing to new location
        clearEditor()

        //  Always save when leaving previous edited cell.
        let modelIndex = MemoryByteModel.modelCellIndex(index)
        tableView.edit(MemoryByteModel.index(modelIndex.row, modelIndex.column))
        //tableView.edit(MemoryByteModel.index(row,col))
        console.log("Single: Index="+ index +": Row=" +modelIndex.row + ", Column:" + modelIndex.column)

        cell.isSelected = MemoryByteModel.setSelected(MemoryByteModel.index(row, column))
        console.log(cell.isSelected)*/
      }

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
}
