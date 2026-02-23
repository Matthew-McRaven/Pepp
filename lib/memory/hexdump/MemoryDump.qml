import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
//  For DelegateChooser
import QtQml.Models

import "." as Ui
import edu.pepp 1.0

Item {
    id: root
    NuAppSettings {
        id: settings
    }
    FontMetrics {
        id: fm
        font: settings.extPalette.baseMono.font
    }
    property int colWidth: 25
    property int rowHeight: 20
    property alias model: tableView.model
    property alias memory: memory.memory
    property alias mnemonics: memory.mnemonics
    property int currentAddress: tableView.currentAddress
    function scrollToAddress(addr) {
        const row = Math.floor(addr / root.model?.bytesPerRow)
        tableView.positionViewAtRow(row, TableView.Contain)
    }

    TableView {
        id: tableView
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: buttonLayout.top

        rowSpacing: 0
        columnSpacing: 0
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        focus: true

        //  Selection information
        selectionBehavior: TableView.SelectCells
        selectionMode: TableView.ContiguousSelection
        editTriggers: TableView.SingleTapped //  Manage editor manually
        model: MemoryModel {
            id: memory
        }

        //  Ascii column must be calculated since byte width per line is configurable
        property int asciiWidth: 10 * (root.model?.bytesPerRow ?? 10)

        //  Used for paging
        property real pageSize: 20 //  Default value, replace on screen resize
        property bool viewResizing: false //  Flag to identify resizing event
        property int partialLine: 0 //  If line is partially blocked, this value will be -1
        property int currentAddress: (topRow > 0 ? topRow : 0) * (root.model?.bytesPerRow
                                                                  ?? 0)

        //  For grid column sizes. Currently, columns are not resizeable
        columnWidthProvider: function (column) {
            if (column === root.model.Column.LineNo)
                return 40 //colWidth * 2
            else if (column === root.model.Column.Border1)
                return 11
            else if (column === root.model.Column.Border2)
                return 11
            else if (column === root.model.Column.Ascii)
                return asciiWidth
            else
                return colWidth
        }

        rowHeightProvider: function (row) {
            return rowHeight
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
            if (viewResizing) {
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
        Keys.onPressed: function (event) {
            event.accepted = keyPress(event.key)
        }

        function keyPress(key) {
            //  Current cell and edit mode. Used below.
            const pt = root.model.currentCell()
            let isEditMode = root.model.isEditMode()


            /*  When editing, cell may move within view without the viewport
      //  moving. For example, arrow down at top of screen will move
      //  cell, but viewport does not move.
      //  Note, these values are not used unless a cell is being edited
      */
            let cellRowLocation = pt.row
            let cellColLocation = pt.column

            //  Review remaining key strokes
            switch (key) {
            case Qt.Key_PageUp:
            {
                //  Make sure first line is fully visible
                moveViewPort(0)

                //  Move up a page, but stop at top
                //  Edit cell moves in synch with main view
                const rowChange = -Math.min(pageSize, tableView.topRow)

                //  Move viewport-MUST OCCUR BEFORE OPENING CELL
                moveViewPort(rowChange)

                //  Determine edit cell location
                cellRowLocation = Math.max(cellRowLocation - rowChange, 0)
                break
            }
            case Qt.Key_PageDown:
            {
                //  Make sure first line is fully visible
                moveViewPort(0)

                //  Move down a page, but stop at bottom
                //  Edit cell moves in synch with main view
                const rowChange = Math.min(
                                    tableView.rows - 1 - tableView.bottomRow,
                                    pageSize)

                //  Move viewport-MUST OCCUR BEFORE OPENING CELL
                moveViewPort(rowChange)

                cellRowLocation = Math.min(cellRowLocation + rowChange,
                                           tableView.rows - 1)
                break
            }
            case Qt.Key_Up:
            {
                //  In edit mode, cursor moves differently
                if (isEditMode) {
                    //  Make sure first line is fully visible
                    moveViewPort(0)

                    //  Decrement row
                    cellRowLocation = cellRowLocation - 1
                    if (tableView.topRow < pt.row) {
                        //  Cell is in view, just move cell up in table
                        break
                    }
                }

                //  When not in edit mode, always move up a line.
                //  In edit mode, we are at top of view, move up a line
                //  Always stop at top
                if (tableView.topRow !== 0)
                    //  Move viewport-MUST OCCUR BEFORE OPENING CELL
                    moveViewPort(-1)

                break
            }
            case Qt.Key_Down:
            {
                //  In edit mode, cursor moves differently
                if (isEditMode) {
                    //  Make sure first line is fully visible
                    moveViewPort(0)

                    //  Increment row
                    cellRowLocation = cellRowLocation + 1

                    //  Ignore partially visible lines
                    if ((tableView.bottomRow + partialLine) > pt.row) {
                        //  Cell is in view, just move cell down in table
                        break
                    }
                }

                //  When not in edit mode, always move down a line.
                //  In edit mode, we are at bottom of view, move down a line
                //  Always stop at bottom
                if (tableView.bottomRow < (tableView.rows - 1)) {
                    //  Move viewport-MUST OCCUR BEFORE OPENING CELL
                    moveViewPort(1)
                }

                break
            }
            case Qt.Key_Home:
            {
                if (tableView.topRow !== 0) {
                    //  Move viewport to top from current location
                    //  MUST OCCUR BEFORE OPENING CELL
                    moveViewPort(-tableView.topRow)
                }

                //  New cell location-upper left
                cellColLocation = root.model.Column.CellStart
                cellRowLocation = 0

                //  Cell will close edit mode before handing control
                //  to parent. If there is a valid last cell, control
                //  was in edit mode when key was entered
                if (root.model.lastCell().row > -1) {
                    //  If move is triggered by control, then edit mode is
                    //  determined by presence of last cell
                    isEditMode = true
                }
                break
            }
            case Qt.Key_End:
            {
                console.log("End key:" + root.model.lastCell().row)
                if (tableView.bottomRow < tableView.rows - 1) {
                    //  Move viewport to bottom from current location
                    //  MUST OCCUR BEFORE OPENING CELL
                    moveViewPort(tableView.rows - tableView.bottomRow)
                }

                //  Cell location should be very last column in last row
                cellColLocation = root.model.Column.CellEnd
                cellRowLocation = tableView.rows - 1

                //  Cell will close edit mode before handing control
                //  to parent. If there is a valid last cell, control
                //  was in edit mode when key was entered
                if (root.model.lastCell().row > -1) {
                    //  If move is triggered by control, then edit mode is
                    //  determined by presence of last cell
                    isEditMode = true
                }
                break
            }
            case Qt.Key_Left:
            {
                //  Cannot test for editMode since tableView arrow keys
                //  only happen when edit is disabled. Check to see if there
                //  is a last cell. If so, based on last cell. Otherwise ignore.
                const oldPt = root.model.lastCell()

                //  If not in edit mode, ignore keystroke
                if (oldPt.row === -1)
                    return false

                //  If move is triggered by control, then edit mode is
                //  determined by presence of last cell
                isEditMode = true

                //  test for general case
                if (root.model.Column.CellStart < oldPt.column) {
                    //  We are not at beginning of row. Just move left in table
                    cellColLocation = oldPt.column - 1
                    cellRowLocation = oldPt.row
                } //  We are in first cell of row
                else {
                    //  Move to last cell in previous row
                    //  Only move if not in first row
                    if (oldPt.row > 0) {

                        //  Set to last column in previous row
                        cellColLocation = root.model.Column.CellEnd
                        cellRowLocation = oldPt.row - 1

                        //  Did we scroll out of current view?
                        if (tableView.topRow > cellRowLocation) {
                            //  Yes, move viewport
                            moveViewPort(-1)
                        }
                    } else {
                        //  We are in first cell in first row. Do not move.
                        cellColLocation = oldPt.column
                        cellRowLocation = oldPt.row
                    }
                }
                break
            }
            case Qt.Key_Right:
            {
                //  Cannot test for editMode since tableView arrow keys
                //  only happen when edit is disabled. Check to see if there
                //  is a last cell. If so, move to last one. Otherwise ignore.
                const oldPt = root.model.lastCell()

                //  If not in edit mode, ignore keystroke
                if (oldPt.row === -1)
                    return false

                //  If move is triggered by control, then edit mode is
                //  determined by presence of last cell
                isEditMode = true

                //  test for general case
                if (root.model.Column.CellEnd > oldPt.column) {
                    //  We are not at end of row. Just move right in table
                    cellColLocation = oldPt.column + 1
                    cellRowLocation = oldPt.row
                } //  We are in last cell of row
                else {
                    //  Move to first cell in next row
                    //  Only move if not in last row
                    if (oldPt.row < (tableView.rows - 1)) {

                        //  Set to last column in previous row
                        cellColLocation = root.model.Column.CellStart
                        cellRowLocation = oldPt.row + 1

                        //  Did we scroll out of current view?
                        if ((tableView.bottomRow + partialLine) < cellRowLocation) {
                            //  Yes, move viewport
                            moveViewPort(1)
                        }
                    } else {
                        //  We are in last cell in last row. Do not move.
                        cellColLocation = oldPt.column
                        cellRowLocation = oldPt.row
                    }
                }

                break
            }
            default:
                //  Continue propogating event
                return false
            }

            //  Open editor in new cell, if already in edit mode
            if (isEditMode) {
                openEditor(cellRowLocation, cellColLocation)
            }

            return true
        }
        // TODO: precalculate in C++ so that we don't do it thousands of times in JS.
        function blendColors(color1, color2, ratio) {
            var r = color1.r * (1 - ratio) + color2.r * ratio
            var g = color1.g * (1 - ratio) + color2.g * ratio
            var b = color1.b * (1 - ratio) + color2.b * ratio
            var a = color1.a * (1 - ratio) + color2.a * ratio

            return Qt.rgba(r, g, b, a)
        }
        //  Used for drawing grid
        delegate: memoryDelegateChooser

        DelegateChooser {
            id: memoryDelegateChooser
            role: "type"

            //  List exceptions first
            //  Line between line number, cells, and ascii columns
            DelegateChoice {
                id: border
                roleValue: "border"
                Ui.MemoryDumpBorder {
                    rowHeight: rowHeight
                    colWidth: colWidth

                    backgroundColor: palette.base
                    foregroundColor: palette.text
                }
            }

            //  Default -- line numbers and ASCII
            DelegateChoice {
                Ui.MemoryDumpReadOnly {
                    rowHeight: rowHeight
                    colWidth: colWidth
                    Component.onCompleted: {
                        updateBackground()
                        highlightChanged.connect(updateBackground)
                    }
                    // Higlight is "state", so must be manually
                    TableView.onReused: {
                        updateBackground()
                        highlightChanged.connect(updateBackground)
                    }
                    TableView.onPooled: {
                        highlightChanged.disconnect(updateBackground)
                    }
                    backgroundColor: palette.base
                    textColor: palette.text
                    text: model.display
                    tooltip: model.toolTip ?? null
                    property int highlight: model.highlight ?? 0
                    function updateBackground() {
                        switch (model.highlight) {
                        case MemoryHighlight.Modified:
                            backgroundColor = Qt.binding(
                                        () => tableView.blendColors(
                                            Qt.rgba(1, 0, 0, 1),
                                            palette.base, .25))
                            break
                        case MemoryHighlight.PC:
                            backgroundColor = Qt.binding(
                                        () => tableView.blendColors(
                                            Qt.rgba(0x3f / 255., 0x51 / 255.,
                                                    0xb5 / 255., 1),
                                            palette.base, .25))
                            break
                        case MemoryHighlight.SP:
                            backgroundColor = Qt.binding(
                                        () => tableView.blendColors(
                                            Qt.rgba(1, 0x98 / 255., 0, 1),
                                            palette.base, .25))
                            break
                        default:
                            backgroundColor = Qt.binding(() => palette.base)
                        }
                    }

                    textAlign: model.textAlign
                    font: fm.font
                }
            }
        }
    }
    GridLayout {
        id: buttonLayout
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        columns: root.width > (4*pcButton.width) ? 4 : 2
        Label {
            text: "Scroll to:"
            color: palette.windowText
        }
        TextField {
            id: addrField
            maximumLength: 6
            Layout.minimumWidth: fm.averageCharacterWidth*(maximumLength + 2)
            Layout.maximumWidth: pcButton.width
            font: fm.font
            text: `0x${root.currentAddress.toString(16).padStart(4, '0').toUpperCase()}`
            validator: RegularExpressionValidator {
                regularExpression: /0x[0-9a-fA-F]{1,4}/
            }
            onEditingFinished: {
                const v = parseInt(text, 16);
                if (v < 0 || v > 0xFFFF)
                    text = "0x0000";
                else
                    root.scrollToAddress(v);
            }
        }
        Button {
            id: spButton
            text: "SP"
            onClicked: root.scrollToAddress(root.memory.sp())
        }
        Button {
            id: pcButton
            text: "PC"
            onClicked: root.scrollToAddress(root.memory.pc())
        }
    }
}
