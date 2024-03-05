import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
//import QtQuick.Controls.Material  //  For colors
import Qt.labs.qmlmodels          //  For DelegateChooser

import "." as Ui
import edu.pepperdine 1.0

Rectangle {
  id: root

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
    selectionBehavior: TableView.SelectCells
    selectionMode: TableView.ContiguousSelection
    editTriggers: TableView.SingleTapped  //  Manage editor manually
                  /*TableView.EditKeyPressed |
                  TableView.SelectedTapped */

    model: MemoryByteModel

    //  Ascii column must be calculated since byte width per line is configurable
    property int asciiWidth: MemoryByteModel ? (MemoryByteModel.BytesPerColumn ?
                             (10 * MemoryByteModel.BytesPerColumn) : 100) : 100

    //  Used for paging
    property real pageSize: 20        //  Default value, replace on screen resize
    property bool viewResizing: false //  Flag to identify resizing event
    property int  partialLine: 0      //  If line is partially blocked, this value will be -1

    //  For grid column sizes. Currently, columns are not resizeable
    columnWidthProvider: function (column) {
      if(column === MemoryByteModel.Column.LineNo) return 40 //colWidth * 2
      else if(column === MemoryByteModel.Column.Border1) return 11
      else if(column === MemoryByteModel.Column.Border2) return 11
      else if(column === MemoryByteModel.Column.Ascii) return asciiWidth
      else return colWidth
    }

    rowHeightProvider: function (row) {return rowHeight}

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
    //  dimensions are not available yet.
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

        //  If last column is partially obsured, do not include in page size
        if (tableView.visibleArea.heightRatio * tableView.rows > pageSize) {
          //  Save partial line indicator for page movements
          partialLine = -1
          pageSize = pageSize - 1
        }
      }
    }

    //  Capture movement keys in table view
    Keys.onPressed: (event) => {
      event.accepted = keyPress(event.key)
    }

    function keyPress(key) {
      //  Current cell and edit mode. Used below.
      const pt = MemoryByteModel.currentCell()
      let isEditMode = MemoryByteModel.isEditMode()

      /*  When editing, cell may move within view without the viewport
      //  moving. For example, arrow down at top of screen will move
      //  cell, but viewport does not move.
      //  Note, these values are not used unless a cell is being edited
      */
      let cellRowLocation = pt.row
      let cellColLocation = pt.column

      //  Review remaining key strokes
      switch(key) {
      case Qt.Key_PageUp: {
        //  Make sure first line is fully visible
        moveViewPort(0)

        //  Move up a page, but stop at top
        //  Edit cell moves in synch with main view
        const rowChange = -Math.min(pageSize,tableView.topRow)

        //  Move viewport-MUST OCCUR BEFORE OPENING CELL
        moveViewPort(rowChange)

        //  Determine edit cell location
        cellRowLocation = Math.max(cellRowLocation-rowChange,0)
        break
      }
      case Qt.Key_PageDown: {
        //  Make sure first line is fully visible
        moveViewPort(0)

        //  Move down a page, but stop at bottom
        //  Edit cell moves in synch with main view
        const rowChange = Math.min(tableView.rows - 1 - tableView.bottomRow, pageSize)

        //  Move viewport-MUST OCCUR BEFORE OPENING CELL
        moveViewPort(rowChange)

        cellRowLocation = Math.min(cellRowLocation+rowChange,tableView.rows - 1)
        break
      }
      case Qt.Key_Up: {
        //  In edit mode, cursor moves differently
        if(isEditMode)
        {
          //  Make sure first line is fully visible
          moveViewPort(0)

          //  Decrement row
          cellRowLocation = cellRowLocation - 1
          if(tableView.topRow < pt.row) {
            //  Cell is in view, just move cell up in table
            break
          }
        }

        //  When not in edit mode, always move up a line.
        //  In edit mode, we are at top of view, move up a line
        //  Always stop at top
        if(tableView.topRow !== 0)
          //  Move viewport-MUST OCCUR BEFORE OPENING CELL
          moveViewPort(-1)

        break
      }
      case Qt.Key_Down: {
        //  In edit mode, cursor moves differently
        if(isEditMode)
        {
          //  Make sure first line is fully visible
          moveViewPort(0)

          //  Increment row
          cellRowLocation = cellRowLocation + 1

          //  Ignore partially visible lines
          if((tableView.bottomRow+partialLine) > pt.row) {
            //  Cell is in view, just move cell down in table
            break
          }
        }

        //  When not in edit mode, always move down a line.
        //  In edit mode, we are at bottom of view, move down a line
        //  Always stop at bottom
        if(tableView.bottomRow < (tableView.rows - 1)) {
          //  Move viewport-MUST OCCUR BEFORE OPENING CELL
          moveViewPort(1)
        }

        break
      }
      case Qt.Key_Home: {
        //console.log("Home key:"+MemoryByteModel.lastCell().row)
        if( tableView.topRow !== 0) {
          //  Move viewport to top from current location
          //  MUST OCCUR BEFORE OPENING CELL
          moveViewPort(-tableView.topRow)
        }

        //  New cell location-upper left
        cellColLocation = MemoryByteModel.Column.CellStart
        cellRowLocation = 0

        //  Cell will close edit mode before handing control
        //  to parent. If there is a valid last cell, control
        //  was in edit mode when key was entered
        if(MemoryByteModel.lastCell().row > -1)
        {
          //  If move is triggered by control, then edit mode is
          //  determined by presence of last cell
          isEditMode = true
        }
        break
      }
      case Qt.Key_End: {
        console.log("End key:"+MemoryByteModel.lastCell().row)
        if( tableView.bottomRow < tableView.rows - 1) {
          //  Move viewport to bottom from current location
          //  MUST OCCUR BEFORE OPENING CELL
          moveViewPort(tableView.rows - tableView.bottomRow)
        }

        //  Cell location should be very last column in last row
        cellColLocation = MemoryByteModel.Column.CellEnd
        cellRowLocation = tableView.rows - 1

        //  Cell will close edit mode before handing control
        //  to parent. If there is a valid last cell, control
        //  was in edit mode when key was entered
        if(MemoryByteModel.lastCell().row > -1)
        {
          //  If move is triggered by control, then edit mode is
          //  determined by presence of last cell
          isEditMode = true
        }
        break
      }
      case Qt.Key_Left: {
        //  Cannot test for editMode since tableView arrow keys
        //  only happen when edit is disabled. Check to see if there
        //  is a last cell. If so, based on last cell. Otherwise ignore.
        const oldPt = MemoryByteModel.lastCell()

        //  If not in edit mode, ignore keystroke
        if(oldPt.row === -1)
          return false

        //  If move is triggered by control, then edit mode is
        //  determined by presence of last cell
        isEditMode = true

        //  test for general case
        if(MemoryByteModel.Column.CellStart < oldPt.column) {
          //  We are not at beginning of row. Just move left in table
          cellColLocation = oldPt.column - 1
          cellRowLocation = oldPt.row
        }
        //  We are in first cell of row
        else {
          //  Move to last cell in previous row
          //  Only move if not in first row
          if(oldPt.row > 0) {

            //  Set to last column in previous row
            cellColLocation = MemoryByteModel.Column.CellEnd
            cellRowLocation = oldPt.row - 1

            //  Did we scroll out of current view?
            if(tableView.topRow > cellRowLocation) {
              //  Yes, move viewport
              moveViewPort(-1)
            }
          }
          else {
            //  We are in first cell in first row. Do not move.
            cellColLocation = oldPt.column
            cellRowLocation = oldPt.row
          }
        }

        //  Set edit mode for for new cell location
        //const index = MemoryByteModel.index(cellRowLocation, cellColLocation)
        //MemoryByteModel.setSelected(index,MemByteRoles.Editing)

        break
      }
      case Qt.Key_Right: {
        //  Cannot test for editMode since tableView arrow keys
        //  only happen when edit is disabled. Check to see if there
        //  is a last cell. If so, move to last one. Otherwise ignore.
        const oldPt = MemoryByteModel.lastCell()

        //  If not in edit mode, ignore keystroke
        if(oldPt.row === -1)
          return false

        //  If move is triggered by control, then edit mode is
        //  determined by presence of last cell
        isEditMode = true

        //  test for general case
        if(MemoryByteModel.Column.CellEnd > oldPt.column) {
          //  We are not at end of row. Just move right in table
          cellColLocation = oldPt.column + 1
          cellRowLocation = oldPt.row
        }
        //  We are in last cell of row
        else {
          //  Move to first cell in next row
          //  Only move if not in last row
          if(oldPt.row < (tableView.rows - 1)) {

            //  Set to last column in previous row
            cellColLocation = MemoryByteModel.Column.CellStart
            cellRowLocation = oldPt.row + 1

            //  Did we scroll out of current view?
            if((tableView.bottomRow + partialLine) < cellRowLocation) {
              //  Yes, move viewport
              moveViewPort(1)
            }
          }
          else {
            //  We are in last cell in last row. Do not move.
            cellColLocation = oldPt.column
            cellRowLocation = oldPt.row
          }
        }

        //  Set edit mode for for new cell location
        //const index = MemoryByteModel.index(cellRowLocation, cellColLocation)
        //MemoryByteModel.setSelected(index,MemByteRoles.Editing)

        break
      }
      default:
        //  Continue propogating event
        return false
      }

      //  Open editor in new cell, if already in edit mode
      if(isEditMode) {
        openEditor(cellRowLocation, cellColLocation)
      }

      return true
    }

    //  Used for drawing grid
    delegate: memoryDelegateChooser

    DelegateChooser {
      id: memoryDelegateChooser
      role: "typeRole"

      //  List exceptions first
      //  Line between line number, cells, and ascii columns
      DelegateChoice {
        id: border
        roleValue: "borderCol"

        Ui.MemoryDumpBorder {
          rowHeight: rowHeight
          colWidth: colWidth

          backgroundColor: model.backgroundColorRole
          foregroundColor: model.textColorRole
        }
      }
      //  Control for presenting line number in first column
      DelegateChoice {
        roleValue: "lineNoCol"

        Ui.MemoryDumpReadOnly {
          rowHeight: rowHeight
          colWidth: colWidth

          backgroundColor: model.backgroundColorRole
          textColor: model.textColorRole
          text: model.lineNoRole
          textAlign: model.textAlignRole
          font: asciiFont
        }
      }
      //  Column representing ascii representation of hex values
      DelegateChoice {
        roleValue: "asciiCol"

        Ui.MemoryDumpReadOnly {
          rowHeight: rowHeight
          colWidth: colWidth

          backgroundColor: model.backgroundColorRole
          textColor: model.textColorRole
          text: model.asciiRole
          textAlign: model.textAlignRole
          font: asciiFont
        }
      }

      //  Default role - Show cell values. Control is editable
      DelegateChoice {
        Ui.MemoryDumpCells {
          id: cell
          rowHeight: rowHeight
          colWidth: colWidth

          backgroundColor: model.backgroundColorRole
          textColor: model.textColorRole
          text: model.byteRole
          textAlign: model.textAlignRole
          font: hexFont

          //  Initialize edit delegate here
          TableView.editDelegate: Ui.MemoryDumpEdit {
            id: ed
            rowHeight: rowHeight
            colWidth: colWidth
            backgroundColor: model.backgroundColorRole
            textColor: model.textColorRole
            text: model.byteRole
            textAlign: model.textAlignRole
            font: hexFont
            editFocus: ed.visible

            //  Javascript cannot see parent tableView. Add as member for
            //  Javascript functions
            parentTable: tableView

            //  Appear in cell being edited
            anchors.fill: cell

            onStartEditing: { //TableView.onCurrentChanged?

              //  Set edit formatting
              const index = MemoryByteModel.index(row, column)
              MemoryByteModel.setSelected(index,MemByteRoles.Editing)
            }

            onFinishEditing: (save) => {
              // Only save if flagged, and values are different
             if(save) {
                if(model.byteRole !== ed.text) {
                  console.log("Model updated")
                  model.byteRole = ed.text
                  cell.text = ed.text
                }
              }

              //  Clear edit formatting
              MemoryByteModel.clearSelected(MemoryByteModel.index(row, column),
                                            MemByteRoles.Editing)
            }

            onDirectionKey: (key) => {
              console.log("TableView.onDirectionKey"+row+","+column+","+
                          ","+key)

              //  Edit control has indicated that user has arrowed out of control
              //  Close current editor.
              tableView.closeEditor()

              //  Edit delegate cannot see table view. Pass tableview
              //  as parameter to access
              parentTable.keyPress(key)
            }
          }
        }
      }
    }

    function openEditor(newRow, newCol) {
      //  moveViewPort may trigger large movement in the
      //  visible table. Force the layout to complete before
      //  triggering editor. Otherwise, cell editor below will fail
      //  because cell may not be rendered yet.
      tableView.forceLayout()

      const index = MemoryByteModel.index(newRow, newCol)
      //  Inform model that index is being edited.
      MemoryByteModel.setSelected(index,MemByteRoles.Editing)

      //  Trigger edit delegate
      tableView.edit(index)
    }

    function moveViewPort(rowOffset)  {
      //  View port is called from many places
      //  When there are partial lines, the top line
      //  may be partially obsured. Set top row to be fully
      //  visible when offset is 0
      if(rowOffset === 0) {
        positionViewAtRow(tableView.topRow,TableView.Contain)
      }
      //  Handle up. Do not scroll past first line
      else if (rowOffset < 0 ) {
        //  Prevent scrolling above beginning table
        const newRow = Math.max(0,tableView.topRow+rowOffset)
        positionViewAtRow(newRow,TableView.Contain)
      }
      //  Handle down. Do not scroll past last line
      else {
        //  Leave 1 page of data on screen
        const newRow = Math.min(tableView.rows-1,
                                tableView.bottomRow+rowOffset)

        positionViewAtRow(newRow,TableView.Contain)
        console.log("newRow=" + newRow+","+ tableView.topRow)
      }
    }
  }
}
