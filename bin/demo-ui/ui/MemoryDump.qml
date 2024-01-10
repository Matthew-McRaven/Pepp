import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import "./Components" as Ui
import edu.pepperdine 1.0

Rectangle {
  id: root
  anchors.fill: parent

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

  property int colWidth: 25
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
    focus: true

    //  Selection information
    selectionBehavior: TableView.SelectCells //TableView.SelectionDisabled
    selectionMode: TableView.ContiguousSelection //TableView.SingleSelection
    editTriggers: TableView.EditKeyPressed |
                  TableView.SelectedTapped |
                  TableView.SingleTapped//TableView.DoubleTapped

    model: MemoryByteModel

    //  Ascii column must be calculated since byte width per line is configurable
    property int asciiWidth: MemoryByteModel.BytesPerColumn ?
                             (10 * MemoryByteModel.BytesPerColumn) : 100

    //  Used for paging
    property real pageSize: 20        //  Default value, replace on screen resize
    property bool viewResizing: false //  Flag to identify resizing event
    property int  partialLine: 0      //  If line is partially blocked, this value will be -1
    property real activeRow: 0        //  Row currently under edit. -1 if no editing.

    //  For grid column sizes. Currently, columns are not resizeable
    columnWidthProvider: function (column) {
      if(column === MemoryByteModel.Column.LineNo) return 40 //colWidth * 2
      else if(column === MemoryByteModel.Column.Border1) return 1
      else if(column === MemoryByteModel.Column.Border2) return 1
      else if(column === MemoryByteModel.Column.Ascii) return asciiWidth
      else return colWidth
    }

    //  Disable horizontal scroll bar
    ScrollBar.horizontal: ScrollBar {
      policy: ScrollBar.AlwaysOff
    }

    //  Enable vertical scroll bar-always on
    ScrollBar.vertical: ScrollBar {
      id: vsc
      policy: ScrollBar.AlwaysOn
    }

    //  This event captures viewport resizing, but new
    //  dimensions are not available yet. Flag for
    visibleArea.onHeightRatioChanged: {
      //  Flag onLayoutChanged to recalculate number of visible rows
      viewResizing = true
    }

    //  Recalculate dimensions when screen is resized
    //  Note, this function treats scrolling as a layout change
    //  Limit updates to just viewport resizing
    onLayoutChanged: {
      //  Calculate screen size for page up/page down
      if(viewResizing) {
        //  Disable resizing after handling event
        viewResizing = false
        pageSize = (tableView.bottomRow - tableView.topRow)
        //console.log("onLayoutChanged: pageSize="+pageSize)

        //  If last column is partially obsured, do not include in page size
        if (tableView.visibleArea.heightRatio * tableView.rows > pageSize) {
          //  Save partial line indicator for page movements
          partialLine = -1
          pageSize = pageSize - 1
        }
        //console.log("onLayoutChanged: pageSize,top,bottom,height="+pageSize + ","+ tableView.topRow +
        //            ","+ tableView.bottomRow + ","+ tableView.visibleArea.heightRatio * tableView.rows)
      }
    }

    //  Capture movement keys in table view
    Keys.onPressed: (event) => {
      console.log("TableView position=" + vsc.position + " pageSize="+pageSize)

      //  Ignore keystrokes in edit mode. Editor handles key strokes
      if(MemoryByteModel.isEditMode())
        return

      //  Review remaining key strokes
      switch(event.key) {
      case Qt.Key_PageUp:
        //console.log("Tableview: Page Up key "+(-pageSize))
        moveViewPort(-pageSize)
        event.accept = true
        break
      case Qt.Key_PageDown:
        //console.log("Tableview: Page Down key "+(pageSize))
        moveViewPort(pageSize)
        event.accept = true
        break
      case Qt.Key_Up:
        moveViewPort(-1)
        event.accept = true
        break
      case Qt.Key_Down:
        moveViewPort(1)
        event.accept = true
        break
      case Qt.Key_Home:
        moveViewPort(-rows)
        event.accept = true
        break
      case Qt.Key_End:
        moveViewPort(rows)
        event.accept = true
        break
      }
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
          //visible: (model.toolTipRole && !editing) ? ma.containsMouse : false
          visible: (!editing && ma.hovered)
          delay: 1000
          text: model.toolTipRole
        }

        //  Used to trigger tool tip
        HoverHandler {
          id: ma
          enabled: true
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
        overwriteMode: false
        echoMode: TextInput.Normal
        focus: true

        //  Limit input to valid hexidecimal values
        validator: RegularExpressionValidator { regularExpression: /[0-9,A-F,a-f]{2}/}

        //  Leave edit mode without saving
        Keys.onEscapePressed: tableView.closeEditor()

        //  Handle key movement inside edit control
        Keys.onPressed: (event) => {
          //console.log("Key: "+ event.key)
          var found = false
          var rowOffset = 0
          var colOffset = 0

          //  Key events that we track at TableView
          switch(event.key) {
          case Qt.Key_Up:
            rowOffset = -1
            found = true
            break;
          case Qt.Key_Down:
            rowOffset = 1
            found = true
            break;
          case Qt.Key_PageUp:
            //console.log("Page Up key "+(-tableView.pageSize))
            rowOffset = -tableView.pageSize
            found = true
            break;
          case Qt.Key_PageDown:
            //console.log("Page Down key "+(tableView.pageSize))
            rowOffset = tableView.pageSize
            found = true
            break;
          case Qt.Key_Left:
            colOffset = -1
            found = true
            break;
          case Qt.Key_Right:
            colOffset = 1
            found = true
            break;
          case Qt.Key_Home:
            console.log("Home key (row,col) " + row +","+column)
            //  Movements are relative to current position
            //colOffset = -column
            rowOffset = -row
            found = true
            break
          case Qt.Key_End:
            console.log("End key (row,col)" + row +","+column)
            //  Movements are relative to current position
            //colOffset = MemoryByteModel.columnCount() - column
            rowOffset = MemoryByteModel.rowCount() - row -1
            found = true
          }

          //  If found, do not buble event up to parent
          event.accept = found
          if(found) {
            //  Save results before moving. If cell leaves viewport,
            //  data in control is lost.
            editor.saveEdits()

            //  This function may move the viewport
            openEditor(rowOffset,colOffset)
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
          //  Store current row
          tableView.activeRow = row
          MemoryByteModel.setSelected(MemoryByteModel.index(row, column),MemByteRoles.Editing)

          //  Get all text for editing
          editor.selectAll()
          text = display.text

          //  Highlight first character for editing
          editor.select(0,1)
        }

        //  This function is only activated if valid number or character
        //  is clicked based on validator. Arrow keys and other
        //  characters are ignored.
        onTextEdited: {
          console.log("onTextChanged: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)

          //  We just edited first character, highlight second character
          if( editor.selectionStart === 0) {
            editor.select(1, 2)
            console.log("start 0: pos,start,end=" + editor.cursorPosition + "," +
                        editor.selectionStart + "," + editor.selectionEnd)
          }
          else if(editor.selectionStart === 1) {
            //  After update, save results and move to next cell
            editor.saveEdits()
            openEditor(0,1)
          }
        }

        //  Performed when enter is pressed
        TableView.onCommit: {
          console.log("OnCommit: index=" + index + " Text=" + text)
          tableView.activeRow = -1

          saveEdits()
          clearEditor()
        }

        Component.onDestruction: {
          console.log("OnCommit: row="+row+" col="+ column)
          //  Remove edit coloring of text and background
          MemoryByteModel.clearSelected(MemoryByteModel.index(row, column),
                                        MemByteRoles.Editing)
        }

        function saveEdits() {
          editor.selectAll()

          //  Check if data changed. If so, update model
          if( model.byteRole !== text) {

            console.log("saveEdits")
            model.byteRole = text
            display.text = text
          }
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
        const lastRowIndex = tableView.rows - 1

        if( rowOffset < 0 && row === 0) {
          //  Capture moving up at beginning of document
          //  Set to first column
          newCol = MemoryByteModel.Column.CellStart
        }
        else if( rowOffset > 0 && lastRowIndex === row) {
          //  Capture moving down at end of document
          //  Move to last column
          newCol = MemoryByteModel.Column.CellEnd
        }
        else {
          //  By default, just move based on offset
          //  Make sure new row is in table
          newRow = Math.max(0,Math.min(lastRowIndex, newRow + rowOffset))
          console.log("openEditor-general newRow,row,rowOffset="+newRow+","+ row+","+rowOffset)
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
          //  Do nothing if last row.
          if(row !== lastRowIndex) {

            //  Set to first column in next row
            newCol = MemoryByteModel.Column.CellStart

            //  Jump to next row.
            newRow = newRow + 1
          }
        }
        else {
          //  By default, just move based on offset
          newCol = newCol + colOffset
        }

        //  Move viewport if row moves outside tableView
        console.log("openEditor-before viewPort newRow,row,rowOffset="+newRow+","+ row+","+rowOffset)
        tableView.moveViewPort(newRow - row)
        var newIndex = MemoryByteModel.index(newRow, newCol)

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
    }

    function moveViewPort(rowOffset)
    {
      //  View port is called from many places
      //  If offset is zero, just return since
      //  view port is not moving
      if(rowOffset === 0) {
        console.log("rowOffset=" + rowOffset)
        return
      }
      //  Cursor moves differently in edit mode
      //  In unedit mode, up and down will always move viewport
      //  In edit mode, up and down only work when scrolling out of viewport

      const isEditMode = MemoryByteModel.isEditMode()
      //console.log("inEdit? " + isEditMode)

      //  Compute new row location to see if in viewport
      const newRow = activeRow + rowOffset
      const rowDelta = rowOffset / tableView.rows

      //console.log("moveViewPort (rowOffset,active,new,top,bottom)=" +
      //            rowOffset + "," + activeRow + "," + newRow + "," +
      //            topRow + "," + bottomRow)

      //  Handle up. Only move if new position is not visible
      //  If no focus, anyways move
      if (rowOffset < 0 &&
          (isEditMode === false || newRow < topRow)) {

        //  Prevent scrolling above beginning table
        if(vsc.position !== 0.0) {
          let view = Math.max(0, vsc.position + rowDelta)
          console.log("Up=" + view)
          vsc.setPosition(view)
        }
      }

      //  Handle down. Only move if new position is not visible
      else if (rowOffset > 0 &&
               (isEditMode === false || newRow > bottomRow + partialLine)) {

        const lastPage = 1 - tableView.visibleArea.heightRatio // tableView.rows
        //console.log( "Page, Rows, ratio=" + pageSize + ","+ tableView.rows +
        //            ","+ lastPage)

        //  Prevent scrolling off of end of table
        //  Leave 1 page of data
        if(vsc.position !== lastPage) {
          let view = Math.min(lastPage,
                              vsc.position + rowDelta)
          console.log("min,calc=" + lastPage + ","+ vsc.position + rowDelta)
          vsc.setPosition(view)
        }
      }
    }
  }
}
