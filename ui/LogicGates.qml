pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage
import QtQuick.Layouts

import CircuitDesign 1.0
import "move.js" as Move

Rectangle {
    id: root

    property real cellWidth: 100

    /*Component.onCompleted: {
        //  Initialize first stamp
        buttonGroup.buttons[0].checked = true;
        currentStamp(0);
    }*/

    //  Static list of gates for selection
    DiagramListModel {
        id: diagramModel
    }

    //  Filter list for properties box
    SortFilterProxyModel {
        id: filterModel
        model: diagramModel

        // Filter based on whether the 'shapeType' role
        filters: [
            FunctionFilter {
                function filter(data: RoleData): bool {
                    return data.shapeType === "Diagram";
                }
            }
        ]
    }
    component RoleData: QtObject {
        property string shapeType
    }

    DiagramPropertyModel {
        id: dataModel
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        //  Diagram buttons
        ColumnLayout {

            SplitView.preferredWidth: root.cellWidth * 2 + source.columnSpacing
            SplitView.maximumWidth: SplitView.preferredWidth
            SplitView.minimumWidth: SplitView.preferredWidth

            DiagramListView {
                id: source

                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
            }

            Item {  //  A spacer
                Layout.fillHeight: true
            }

            DiagramProperties {
                id: props
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: true

                diagramModel: diagramModel.currentDiagram
                model: filterModel
            }

        }

        Item {
            //  Background used as canvas for object placement
            //  Background does not interact with mouse or diagrams
            GridView {
                anchors.fill: parent
                delegateModelAccess: DelegateModel.ReadOnly
                model: 1000//Math.floor(root.height/100) * Math.floor(root.width/100)

                delegate: GridLine {
                    width: 100
                    height: 100
                }
            }

            Rectangle {
                id: canvas
                anchors.fill: parent

                property int curIndex: -1
                property var curStamp: source.currentStamp
                property Diagram fromObject: null
                property Diagram toObject: null

                color: "transparent" //"gainsboro"

                DiagramSelectionModel {
                    id: selectModel
                    behavior: DiagramSelectionModel.SelectSingle
                    model: dataModel
                }

                //  Test Only Diagrams for checking line connection
                Diagram {
                    id: diagram1

                    Component.onCompleted: {
                        model.name = "AND Gate"
                        model.imageSource = "qrc:/and"
                    }
                    x: 100
                    y: 100
                    z: 1
                    selectModel: selectModel
                }

                Diagram {
                    id: diagram2

                    Component.onCompleted: {
                        model.name = "OR Gate";
                        model.imageSource = "qrc:/or";
                        props.diagramModel = model;
                    }

                    x: 200
                    y: 200
                    z: 1
                    selectModel: selectModel
                }

                Line {
                    id: line

                    fromObject: diagram1
                    toObject: diagram2
                }

                //  Used to show where objects will be stamped on canvas
                Rectangle {
                    id: stamp
                    color: "transparent"
                    border {
                        color: "blue"
                        width: 1
                    }
                    width: Move.blockWidth
                    height: Move.blockHeight
                    visible: canvas.curStamp != null
                }

                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: event => {
                        //  No template selected. Just return
                        if (canvas.curStamp === null) {
                            return;
                        }

                        //  Find current stamp
                        root.currentStamp(canvas.curIndex);

                        //  Create diagram
                        var diagram = Move.createBlock(canvas);

                        diagram.model.name = canvas.curStamp.name;
                        diagram.model.imageSource = canvas.curStamp.file;
                        diagram.model.type = canvas.curStamp.key;


                        //  Move object to final spot
                        Move.moveObjectTo(diagram, event.x, event.y);
                        props.diagramModel = diagram.model;

                        //console.log( "onClick1 diagram.x", diagram.x, "diagram.y", diagram.y, "canvas.x", canvas.x, "canvas.y", canvas.y);
                        //console.log( "onClick2 x", event.x, "y", event.y, "stamp.x",stamp.x, "stamp.y", stamp.y);
                    }

                    onPositionChanged: event => {
                        //  Move object within grid (large axis)
                        Move.moveObjectTo(stamp, event.x, event.y);
                        //console.log( "x", event.x, "y", event.y, "stamp.x", stamp.x, "stamp.y", stamp.y);
                    }

                    onEntered: {
                        //  Make grid triangle visible
                        stamp.visible = true;
                    }
                    onExited: {
                        //  Make grid triangle invisible
                        stamp.visible = false;
                    }
                }
            }   //  Rectangle
        }   //  Item
    }
}
