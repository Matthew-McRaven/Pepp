import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
//import QtQuick.Controls.Material  //  For colors
import Qt.labs.qmlmodels          //  For DelegateChooser

import "." as Ui
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
    selectionBehavior: TableView.SelectCells
    selectionMode: TableView.ContiguousSelection
    editTriggers: TableView.SingleTapped  //  Manage editor manually
                  /*TableView.EditKeyPressed |
                  TableView.SelectedTapped */

    model: MemoryByteModel

    //  Ascii column must be calculated since byte width per line is configurable
    property int asciiWidth: MemoryByteModel.BytesPerColumn ?
                             (10 * MemoryByteModel.BytesPerColumn) : 100

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
        //console.log("onLayoutChanged: pageSize,top,bottom,height="+pageSize + ","+ tableView.topRow +
        //            ","+ tableView.bottomRow + ","+ tableView.visibleArea.heightRatio * tableView.rows)
      }
    }

    onActiveFocusChanged: {
      console.log("tableView.onActiveFocusChanged: " + focus)
    }

    //  Capture movement keys in table view
    Keys.onPressed: (event) => {
      console.log("TableView keys " + event.accepted)

      //  Current and previous cell
      const pt = MemoryByteModel.currentCell()
      let rowOffset = 0 //  Assume no change in row

      /*  When editing, cell may move within view without the viewport
      //  moving. For example, arrow down at top of screen will move
      //  cell, but viewport does not move.
      //  Note, these values are not used unless a cell is being edited
      */
      let cellRowOffset = 0
      let cellColOffset = 0

      //  Review remaining key strokes
      switch(event.key) {
      case Qt.Key_PageUp: {
        rowOffset = -Math.min(pageSize,tableView.topRow)
        break
        /*moveViewPort(-pageSize)
        if(MemoryByteModel.isEditMode()) {
          const newRow = Math.max(0, pt.row - pageSize)
          //console.log("Tableview: Page up "+pageSize+","+newRow+","+pt.column)

          openEditor(newRow,pt.column)
        }
        event.accept = true*/
      }
      case Qt.Key_PageDown: {
        //console.log("Tableview: Page Down key "+(pageSize))
        //rowOffset = Math.min(pageSize,
        //break

        moveViewPort(pageSize)
        if(MemoryByteModel.isEditMode()) {
          const newRow = Math.min(tableView.rows - 1, pt.row + pageSize)
          //console.log("Tableview: Page Down "+pageSize+","+newRow+","+pt.column)

          openEditor(newRow,pt.column)
        }
        event.accept = true
        return
      }
      case Qt.Key_Up:
        if(MemoryByteModel.isEditMode()) {
          const newRow = Math.max(0, pt.row - 1)
          console.log("Tableview: Key up " +tableView.topRow+">"+
                      newRow +","+pt.column)

          //  Did we scroll out of current view?
          if(tableView.topRow > newRow)
            //  Yes, move viewport
            moveViewPort(-1)

          openEditor(newRow,pt.column)
        } else {
          moveViewPort(-1)
        }

        event.accept = true
                        return
      case Qt.Key_Down:
        if(MemoryByteModel.isEditMode()) {
          const newRow = Math.min(tableView.rows - 1, pt.row + 1)
          //console.log("Tableview: Key down " +(tableView.bottomRow+ partialLine)+"<"+
          //              newRow +","+pt.column)

          //  Did we scroll out of current view? Exclude partial rows
          if((tableView.bottomRow + partialLine) < newRow) {
            //  Yes, move viewport
            console.log("MoveViewport = 1")
            moveViewPort(1)
          }
          openEditor(newRow,pt.column)
        } else {
          moveViewPort(1)
        }
        event.accept = true
                        return
      case Qt.Key_Home:
        console.log("Tableview: Home"+tableView.topRow)
        moveViewPort(-tableView.topRow-1)
        if(MemoryByteModel.isEditMode()) {
          const newRow = Math.min(tableView.rows - 1, pt.row + pageSize)

          //  If in first row, move to beginning of line
          if( pt.row === 0) {
            openEditor(0,MemoryByteModel.Column.CellStart)
          }
          else {
            //  Move up to first page but keep same column
            openEditor(0,pt.column)
          }
        }
        event.accept = true
                        return
      case Qt.Key_End:
        moveViewPort(rows)
        if(MemoryByteModel.isEditMode()) {
          const newRow = Math.min(tableView.rows - 1, pt.row + pageSize)
          console.log("Tableview: Home")

          //  If in last row, move to end of line
          if( pt.row === (tableView.rows - 1)) {
            openEditor(tableView.rows - 1,MemoryByteModel.Column.CellEnd)
          }
          else {
            //  Move down to last page but keep same column
            openEditor(tableView.rows - 1,pt.column)
          }
        }
        event.accept = true
                        return

      case Qt.Key_Left:
      case Qt.Key_Right:
        //  Right and left keys only works in edit mode when switching
        //  between cells
        console.log("Tableview Right/Left key")
        directionKey(event.key)
        event.accepted = true
        return

      case Qt.Key_F2:
        openEditor(Math.max(pt.row,tableView.topRow),
                   Math.max(pt.column,MemoryByteModel.Column.CellStart))
        event.accept = true
                        return
      default:
        //  Continue propogating event
        event.accepted = false
        return
      }

      //  Move viewport
      moveViewPort(rowOffset)

      //  Open editor if already in edit mode
      if(MemoryByteModel.isEditMode()) {
        const newRow = Math.max(0, pt.row - pageSize)
        //console.log("Tableview: Page up "+pageSize+","+newRow+","+pt.column)

        openEditor(pt.row + rowOffset, pt.column)
      }

      event.accepted = true
    }

    function directionKey(key) {
      const oldPt = MemoryByteModel.lastCell()
      console.log("tv.directionKey="+key+","+
                  oldPt.row+","+oldPt.column)

      //  Cannot test for editMode since tableView arrow keys
      //  only have when edit is disabled. Check to see if there
      // is a last cell. If so, move to last one. Otherwise ignore.
      if(oldPt.row === -1) {
        console.log("Old index is invalid")
        return
      }

      let colOffset = 0  //  Assume no change in column
      let rowOffset = 0  //  Assume no change in row

      //  Left key only works in edit mode when switching
      //  between cells
      if(key === Qt.Key_Left) {

        //  Special rule for first row
        if(MemoryByteModel.Column.CellStart === oldPt.column) {
          //  Only move if not in first cell
          if(oldPt.row > 0)  {

            //  Set to last column
            colOffset = MemoryByteModel.Column.CellEnd -
                        MemoryByteModel.Column.CellStart

            //  Capture moving backwards at beginning of row
            rowOffset = -1

            //  Did we scroll out of current view?
            if(tableView.topRow > (oldPt.row+rowOffset))
              //  Yes, move viewport
              moveViewPort(-1)
          }
        }
        //  No change in row
        else {
          colOffset = -1  //  Assume moving left in table
        }
      }
      //  Right key only works in edit mode when switching
      //  between cells
      else if(key === Qt.Key_Right) {
        //  Special rule for first cell
        if(MemoryByteModel.Column.CellEnd === oldPt.column) {
          //  Only move if not in first cell
          if(oldPt.row < (tableView.rows - 1)) {
            //  Set to first column
            colOffset = MemoryByteModel.Column.CellStart -
                        MemoryByteModel.Column.CellEnd

            //  Capture moving forward at beginning of row
            rowOffset = 1

            //  Did we scroll out of current view? Exclude partial rows
            if((tableView.bottomRow + partialLine) < oldPt.row) {
              //  Yes, move viewport
              moveViewPort(1)
            }
          }
        }
        //  No change in row
        else {
          colOffset = 1  //  Assume moving right in table
        }
      }

      //  Display editor
      console.log("After-newCol,newRow="+","+(oldPt.row + rowOffset)+","+
                  (oldPt.column + colOffset))
      openEditor(oldPt.row + rowOffset, oldPt.column + colOffset)
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
            parentTable: tableView


            //  Appear in cell being edited
            anchors.fill: cell

            onStartEditing: { //TableView.onCurrentChanged?
              console.log("TableView.onStartEditing:"+row+","+column)

              //  Set edit formatting
              const index = MemoryByteModel.index(row, column)
              MemoryByteModel.setSelected(index,MemByteRoles.Editing)
            }

            onFinishEditing: (save) => {
              console.log("TableView.onFinishEditing:"+row+","+column+","+
                          ed.text+","+model.byteRole+","+save)

               // Only save if flagged, and values are different
               if(save ) {
                  if(model.byteRole !== ed.text) {
                    console.log("Model updated")
                    model.byteRole = ed.text
                    cell.text = ed.text
                  }

                  //  Save last cell location
                  //tableView.lastRow = row
                  //tableView.lastCol = column
                 }
              //  Reset formatting
              MemoryByteModel.clearSelected(MemoryByteModel.index(row, column),
                                            MemByteRoles.Editing)
            }

            onDirectionKey: (key) => {
              console.log("TableView.onDirectionKey"+row+","+column+","+
                          ","+key)

              //  Save last cell location
              //tableView.lastRow = row
              //tableView.lastCol = column

              //  Edit control has indicated that user has arrowed out of control
              //  Close current editor.
              tableView.closeEditor()

              //  Edit delegate cannot see table view. Pass tableview
              //  as parameter to access
              parentTable.directionKey(key)
            }
          }
        }
      }
    }

    function openEditor(newRow, newCol) {
      console.log("openEditor-newRow,newCol="+newRow+","+ newCol)
      const index = MemoryByteModel.index(newRow, newCol)
      MemoryByteModel.setSelected(index,MemByteRoles.Editing)
      tableView.edit(index)
    }

    function moveViewPort(rowOffset)  {
      //  View port is called from many places
      //  If offset is zero, just return since
      //  view port is not moving
      if(rowOffset === 0) {
        console.log("rowOffset=" + rowOffset)
        return
      }

      //  Compute new row location to see if in viewport
      const rowDelta = rowOffset / tableView.rows

      //  Handle up. Do not scroll past first line
      if (rowOffset < 0 ) {

        //  Prevent scrolling above beginning table
        if(vsc.position !== 0.0) {
          let view = Math.max(0, vsc.position + rowDelta)
          //console.log("Up=" + view)
          vsc.setPosition(view)
        }
      }

      //  Handle down. Do not scroll past last line
      else if (rowOffset > 0 ) {

        //  Leave 1 page of data on screen
        const lastPage = 1 - tableView.visibleArea.heightRatio

        //  Prevent scrolling off of end of table
        if(vsc.position !== lastPage) {
          const view = Math.min(lastPage,
                              vsc.position + rowDelta)
          //console.log("min,calc=" + lastPage + ","+ vsc.position + rowDelta)
          vsc.setPosition(view)
        }
      }
    }
  }
}
