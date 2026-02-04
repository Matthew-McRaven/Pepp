pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

import CircuitDesign
import DiagramEnum

Item {
    id: root
    property var currentStamp: null
    property var currentIndex: tableView.selectionModel.currentIndex //selectedItem()
    property alias dataModel: tableView.model

    function deleteItem(index) {
        root.dataModel.clearItemData(index);
        root.dataModel.update(index);
    }

    function addItem(index, file, role=DiagramProperty.ImageSource) {
        root.dataModel.setData(index, file, role);
        root.dataModel.update(index);
    }

    ScrollView {
        id: scrollView
        anchors.fill: root

        contentItem: TableView {
            id: tableView

            clip: true
            columnSpacing: 0
            rowSpacing: 0
            alternatingRows: false
            reuseItems: true
            boundsBehavior: Flickable.StopAtBounds
            selectionBehavior: TableView.SelectCells // TableView.SingleSelection
            selectionMode: TableView.ExtendedSelection
            selectionModel: selectionModel

            delegate: Diagram {
                id: tvDelegate
                implicitWidth: 100
                implicitHeight: 100
                //anchors.centerIn: parent
                source: tvDelegate.model.imageSource !== undefined ? tvDelegate.model.imageSource : ""

                /*DragHandler {
                    id: dragHandler
                }*/
            }

            /*DiagramTableViewDelegate {
                id: tvDelegate
                implicitWidth: 100
                implicitHeight: 100
                color: "white"

                background: GridLines {
                    current: tvDelegate.current
                }
            }*/

            /*TableViewDelegate {
                id: tvDelegate

                implicitWidth: 100
                implicitHeight: 100
                leftPadding: 0
                topPadding: 0

                /*onClicked: {
                    //  If there is no stamp, do not update model
                    if(root.currentStamp === null)
                        return;

                    const index = tableView.index(row, column);
                    const hasData = tableView.model.data(index, Qt.DisplayRole) !== undefined;

                    //  Update model with current stamp
                    if(!hasData)
                        tableView.model.setData(index, root.currentStamp.qrcFile, DiagramProperty.ImageSource);
                    tableView.selectionModel.select(index, ItemSelectionModel.SelectCurrent);
                }

                background: GridLines {
                    current: tvDelegate.current
                }

                contentItem: Diagram {
                    width: 100
                    height: 100
                    source: tvDelegate.model.imageSource !== undefined ? tvDelegate.model.imageSource : ""
                }
            }*/

            MouseArea {
                id: dragArea

                property point dragCell: Qt.point(-1, -1)
                property bool hadSelection: false

                anchors.fill: parent
                drag.axis: Drag.XandYAxis
                drag.target: dropArea
                acceptedButtons: Qt.LeftButton
                cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.ArrowCursor

                onPressed: mouse => {
                    //console.log("tableview.onPressed", mouse.x, mouse.y);

                    mouse.accepted = false;
                    const position = Qt.point(mouse.x, mouse.y);
                    const cell = tableView.cellAtPosition(position, true);
                    if (cell.x < 0 || cell.y < 0)
                        return;  //  Invalid cell location

                    // check selected indexes
                    const index = tableView.index(cell.y, cell.x);
                    const url = tableView.model.data(index, DiagramProperty.ImageSource);
                    const hasData = url !== undefined;

                    //  Update model with current stamp
                    //  If there is no stamp, do not update model
                    if(!hasData && root.currentStamp !== null) {
                        root.addItem(index, root.currentStamp.qrcFile);

                    } else {
                        //  Otherwise, we are dragging current cell
                        mouse.accepted = true;
                        dragCell = cell;
                        dragImage.source = url;
                        dropArea.startDragging();
                    }

                    //  Highlight selected cell
                    tableView.selectionModel.select(index, ItemSelectionModel.SelectCurrent);
                }

                onReleased: mouse => {
                    //console.log("tableview.onReleased", mouse.x, mouse.y);
                    dropArea.stopDragging()


                    hadSelection = false
                    dragCell = Qt.point(-1, -1)
                }
            }

            Keys.onPressed: event => {
                console.log("Keys.onPressed", event);
                /*if (event.matches(StandardKey.Copy)) {
                    copyToClipboard()
                } else if (event.matches(StandardKey.Cut)) {
                    cutToClipboard()
                } else if (event.matches(StandardKey.Paste)) {
                    pasteFromClipboard()
                } else*/
                if (event.matches(StandardKey.Delete)) {
                    //visibleCellsConnection.blockConnection()
                    if (tableView.selectionModel.hasSelection)
                        model.clearItemData(tableView.selectionModel.selectedIndexes)
                    else {
                        root.deleteItem(tableView.selectionModel.currentIndex);
                        /*const index = tableView.selectionModel.currentIndex;
                        model.clearItemData(index);
                        root.dataModel.update(index);*/
                    }
                    //visibleCellsConnection.blockConnection(false)
                    //visibleCellsConnection.updateViewArea()
                }
            }   //  Keys.onPressed
        }   //  TableView
    }   //  ScrollView

    VectorImage {
        id: dragImage
        width: 75
        height: 75

        opacity: .5
        visible: dragArea.drag.active
        source: "qrc:/inverter"
        fillMode: Image.PreserveAspectFit
        preferredRendererType: VectorImage.CurveRenderer
    }

    DropArea {
        id: dropArea

        property point dropCell: Qt.point(-1, -1)
        // This property keeps the interactive value to restore it after
        // dragging is finished. The reason is that when the interactive
        // mode is true, it steals the events and prevents the drop area
        // from working.
        property bool restoreInteractiveValue: tableView.interactive

        anchors.fill: parent
        Drag.active: dragArea.drag.active
        enabled: dragArea.drag.active

        function startDragging()
        {
            restoreInteractiveValue = tableView.interactive
            tableView.interactive = false
        }

        function stopDragging()
        {
            Drag.drop()
            tableView.interactive = restoreInteractiveValue
        }

        onDropped: {
            console.log("tableview.onDropped", dragArea.mouseX, dragArea.mouseY);
            const position = Qt.point(dragArea.mouseX, dragArea.mouseY)
            dropCell = tableView.cellAtPosition(position, true)
            if (dropCell.x < 0 || dropCell.y < 0)
                return
            if (dragArea.dragCell === dropCell)
                return

            /*if (!isDropPossible(dropCell)) {
                tableView.model.clearHighlight()
                return
            }*/

            //tableView.model.clearItemData(_spreadSelectionModel.selectedIndexes)
            /*for (let i = 0; i < mimeDataProvider.size(); ++i) {
                let cell = mimeDataProvider.cellAt(i)
                cell.x += dropCell.x - dragArea.dragCell.x
                cell.y += dropCell.y - dragArea.dragCell.y
                const index = tableView.index(cell.y, cell.x)
                mimeDataProvider.saveDataToModel(i, index, tableView.model)
            }
            mimeDataProvider.reset()
            _spreadSelectionModel.clearSelection()
            */
            const dragIndex = tableView.index(dragArea.dragCell.y, dragArea.dragCell.x);
            const dropIndex = tableView.index(dropCell.y, dropCell.x)

            //  Get old data
            const dragData = tableView.model.itemData(dragIndex);

            //  Copy old item to new item, and delete old item
            root.addItem(dropIndex, dragData.imageSource);
            root.deleteItem(dragIndex);

            tableView.selectionModel.select(dropIndex, ItemSelectionModel.SelectCurrent);

            //tableView.model.clearHighlight()
        }

        onPositionChanged: {
            console.log("tableview.onPositionChanged", dragArea.mouseX, dragArea.mouseY);

            //  Show drag image
            dragImage.x = dragArea.mouseX;
            dragImage.y = dragArea.mouseY;

            const position = Qt.point(dragArea.mouseX, dragArea.mouseY)
            // cell is the cell that currently mouse is over it
            const cell = tableView.cellAtPosition(position, true)
            // dropCell is the cell that it was under the mouse's last position
            // if the last and current cells are the same, then there is no need
            // to update highlight, as nothing is changed since last time.
            if (cell === dropCell)
                return

            //if (!isDropPossible(cell)) {
            //    tableView.model.clearHighlight()
            //    return
            //}

            // if something is changed, it means that if the current cell is changed,
            // then clear highlighted cells and update the dropCell.
            //tableView.model.clearHighlight()
            dropCell = cell
            // if the current cell was invalid (mouse is out side of the TableView)
            // then no need to update highlight
            if (cell.x < 0 || cell.y < 0)
                return
            // if dragged cell is the same as the (possibly) dropCell
            // then no need to highlight any cells
            if (dragArea.dragCell === dropCell)
                return

            tableView.model.update(cell.x, cell.y);
            // if the dropCell is not the same as the dragging cell and also
            // is not the same as the cell at the mouse's last position
            // then highlights the target cells
            /*for (let i in _spreadSelectionModel.selectedIndexes) {
                const old_index = _spreadSelectionModel.selectedIndexes[i]
                let cell = tableView.cellAtIndex(old_index)
                cell.x += dropCell.x - dragArea.dragCell.x
                cell.y += dropCell.y - dragArea.dragCell.y
                const new_index = tableView.index(cell.y, cell.x)
                tableView.model.setHighlight(new_index, true)
            }*/
        }
    }   //  DropArea

    ItemSelectionModel {
        id: selectionModel
    }

    SelectionRectangle {
        id: selectionRectangle
        target: tableView
        selectionMode: SelectionRectangle.Auto
    }
}   //  Item
