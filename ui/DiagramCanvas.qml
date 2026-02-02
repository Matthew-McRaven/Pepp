pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import CircuitDesign
import DiagramEnum

Item {
    id: root
    property var currentStamp: null
    property var currentIndex: tableView.selectionModel.currentIndex //selectedItem()
    property alias dataModel: tableView.model

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

            delegate: TableViewDelegate {
                id: tvDelegate

                implicitWidth: 100
                implicitHeight: 100
                leftPadding: 0
                topPadding: 0

                onClicked: {
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
            }

            Keys.onPressed: function (event) {
                /*if (event.matches(StandardKey.Copy)) {
                    copyToClipboard()
                } else if (event.matches(StandardKey.Cut)) {
                    cutToClipboard()
                } else if (event.matches(StandardKey.Paste)) {
                    pasteFromClipboard()
                } else*/ if (event.matches(StandardKey.Delete)) {
                    //visibleCellsConnection.blockConnection()
                    if (tableView.selectionModel.hasSelection)
                        model.clearItemData(tableView.selectionModel.selectedIndexes)
                    else {
                        const index = tableView.selectionModel.currentIndex;
                        model.clearItemData(index);
                        root.dataModel.update(index.row, index.column);
                    }
                    //visibleCellsConnection.blockConnection(false)
                    //visibleCellsConnection.updateViewArea()
                }
            }
            MouseArea {
                id: dragArea

                property point dragCell: Qt.point(-1, -1)
                property bool hadSelection: false

                anchors.fill: parent
                //drag.axis: Drag.XandYAxis
                //drag.target: dropArea
                acceptedButtons: Qt.LeftButton
                //cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.ArrowCursor

                onPressed: mouse => {
                    console.log("Parent onPressed");
                }

                /*onPressed: function (mouse) {
                    //console.log(mouse);
                    mouse.accepted = false;
                    // only when Alt modifier is pressed
                    if (mouse.modifiers !== Qt.AltModifier)
                        return;

                    // check cell under press position
                    const position = Qt.point(mouse.x, mouse.y);
                    const cell = view.cellAtPosition(position, true);
                    console.log(position, cell);
                    if (cell.x < 0 || cell.y < 0)
                        return;
                    // check selected indexes
                    const index = view.index(cell.y, cell.x);
                    hadSelection = selectionModel.hasSelection;
                    if (!hadSelection)
                        selectionModel.select(index, ItemSelectionModel.Select);
                    if (!selectionModel.isSelected(index))
                        return;
                    // store selected data
                    /*mimeDataProvider.reset()
                    mimeDataProvider.loadSelectedData()
                    // accept dragging
                    if (mimeDataProvider.size() > 0) {
                        mouse.accepted = true
                        dragCell = cell
                    }

                    //dropArea.startDragging()
                }*/

                /*onClicked: mouse => {
                    //  No template selected. Just return
                    if (root.currentStamp === null) {
                        return;
                    }
                    // Alt modifier indicates drag/drop. Return
                    if (mouse.modifiers === Qt.AltModifier)
                    return;

                    // check cell under press position
                    const position = Qt.point(mouse.x, mouse.y);
                    const cell = view.cellAtPosition(position, true);
                    console.log(position, cell);
                    if (cell.x < 0 || cell.y < 0)
                    return;
                    // check selected indexes
                    const index = view.index(cell.y, cell.x);
                }*/

                /*onReleased: {
                    //dropArea.stopDragging()
                    // reset selection, if dragging caused the selection
                    if (!hadSelection)
                    selectionModel.clearSelection();
                    hadSelection = false;
                    dragCell = Qt.point(-1, -1);
                }*/
            }   //  MouseArea

        }   //  TableView
    }   //  ScrollView

    ItemSelectionModel {
        id: selectionModel
        //onCurrentChanged: root.selectedItem();
        //onSelectionChanged: root.selectedItem();
    }

    SelectionRectangle {
        id: selectionRectangle
        target: tableView
        selectionMode: SelectionRectangle.Auto
    }
}   //  Item
