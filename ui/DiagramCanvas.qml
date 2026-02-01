pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import CircuitDesign

Item {
    id: root
    property var currentStamp: null

    ScrollView {
        id: scrollView
        anchors.fill: root

        contentItem: TableView {
            id: tableView

            property var component: null

            //anchors.fill: parent
            clip: true
            columnSpacing: 0
            rowSpacing: 0
            alternatingRows: false
            reuseItems: false // temporary workaound
            boundsBehavior: Flickable.StopAtBounds
            selectionBehavior: TableView.SelectCells // TableView.SingleSelection
            selectionMode: TableView.ExtendedSelection
            selectionModel: selectionModel
            model: DiagramDataModel {
                id: diagramModel
            }

            delegate: TableViewDelegate {
                id: tvDelegate

                implicitWidth: 100
                implicitHeight: 100
                leftPadding: 0
                topPadding: 0

                onClicked: {
                    console.log("clicked tvDelegate", root.currentStamp);

                    //  If there is no stamp, do not update model
                    if(root.currentStamp === null)
                        return;

                    //console.log("clicked tvDelegate", row, column, tvDelegate.model);
                    const index = tableView.index(row, column);
                    tableView.model.setData(index, root.currentStamp.qrcFile, 258);
                    //console.log(tableView.model.data(index,258));
                    //console.log(tvDelegate.model.imageSource);
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

            //  Just update visible area
            Connections {
                id: visibleCellsConnection
                target: diagramModel

                /*function onDataChanged(tl, br, roles)
                {
                    updateViewArea()
                }*/

                // The model is updated, then the visible area needs to be updated as well.
                // Maybe some cells need to get the display data again
                // due to their data, if it's a formula.
                function updateViewArea()
                {
                    for (const row = tableView.topRow; row <= tableView.bottomRow; ++row) {
                        for (const column = tableView.leftColumn; column <= tableView.rightColumn; ++column) {
                            diagramModel.update(row, column);
                        }
                    }
                }

                // Blocks/unblocks the connection. This function is useful when
                // some actions may update a large amount of data in the model,
                // or may update a cell which affects other cells,
                // for example clipboard actions and drag/drop actions.
                // Block the connection, update the model, unblock the connection,
                // and the call the updateViewArea() function to update the view.
                function blockConnection(block=true)
                {
                    visibleCellsConnection.enabled = !block
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
                    console.log("Has selection", tableView.selectionModel.hasSelection);
                    if (tableView.selectionModel.hasSelection)
                        model.clearItemData(tableView.selectionModel.selectedIndexes)
                    else {
                        const index = tableView.selectionModel.currentIndex;
                        console.log("Curent index", index);
                        model.clearItemData(index);
                        diagramModel.update(index.row, index.column);
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

                onClicked: mouse => {
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

                    //  Find current stamp
                    //root.currentStamp(canvas.curIndex);

                    //  Create diagram
                    /*var diagram = root.createBlock(canvas);

                    diagram.model.name = root.currentStamp.name;
                    diagram.model.imageSource = root.currentStamp.file;
                    diagram.model.type = root.currentStamp.key;

                    //  Move object to final spot
                    Move.moveObjectTo(diagram, event.x, event.y);
                    props.diagramModel = diagram.model;*/

                    //console.log( "onClick1 diagram.x", diagram.x, "diagram.y", diagram.y, "canvas.x", canvas.x, "canvas.y", canvas.y);
                    //console.log( "onClick2 x", event.x, "y", event.y, "stamp.x",stamp.x, "stamp.y", stamp.y);
                }

                /*onReleased: {
                    //dropArea.stopDragging()
                    // reset selection, if dragging caused the selection
                    if (!hadSelection)
                    selectionModel.clearSelection();
                    hadSelection = false;
                    dragCell = Qt.point(-1, -1);
                }*/
            }   //  MouseArea

            /*function createDiagram(canvas) {
            //  Cache component for creating diagrams
            if (root.component === null)
                root.component = Qt.createComponent("Diagram.qml");

            //  Create instance of a diagram and place at indicated x,y coordinate
            var diagram;
            if (root.component.status === Quick.Component.Ready) {
                diagram = root.component.createObject(canvas);
                if (diagram === null) {
                    console.log("Error creating diagram");
                    console.log(component.errorString());
                    return null;
                }
                //diagram.select(canvas.select);
            } else {
                console.log("Error loading individual diagram");
                console.log(component.errorString());
            }
            return diagram;
        }*/
        }   //  TableView
    }   //  ScrollView

    ItemSelectionModel {
        id: selectionModel
        //behavior: SpreadSelectionModel.SelectCells
    }

    SelectionRectangle {
        id: selectionRectangle
        target: tableView
        selectionMode: SelectionRectangle.Auto
    }
}   //  Item
