pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage

import CircuitDesign 1.0
import "move.js" as Move

Rectangle {
    id: root

    property real cellWidth: 128

    ListModel {
        id: diagramModel
        ListElement {
            key: "AND"
            shapeType: "Diagram"
            name: "AND Gate"
            file: "qrc:/and"
        }
        ListElement {
            key: "OR"
            shapeType: "Diagram"
            name: "OR Gate"
            file: "qrc:/or"
        }
        ListElement {
            key: "Inverter"
            shapeType: "Diagram"
            name: "Inverter"
            file: "qrc:/inverter"
        }
        ListElement {
            key: "NAND"
            shapeType: "Diagram"
            name: "NAND Gate"
            file: "qrc:/nand"
        }
        ListElement {
            key: "NOR"
            shapeType: "Diagram"
            name: "NOR Gate"
            file: "qrc:/nor"
        }
        ListElement {
            key: "XOR"
            shapeType: "Diagram"
            name: "XOR Gate"
            file: "qrc:/xor"
        }
        ListElement {
            key: "Line"
            shapeType: "line"
            name: "Line"
            file: "qrc:/line"
        }
        ListElement {
            key: "Multiline"
            shapeType: "Line"
            name: "Multiline"
            file: "qrc:/multiline"
        }
        ListElement {
            key: "Bus"
            shapeType: "line"
            name: "Bus"
            file: "qrc:/bus"
        }
    }

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

    //  Temporary for testing
    DiagramProperties {
        id: props
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        z: 2

        //diagram: canvas.diagram2
        model: filterModel
    }

    DiagramPropertyModel {
        id: dataModel
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        ButtonGroup {
            buttons: source.children.filter(child => child !== rep)

            onClicked: btn => {
                canvas.curIndex = btn.index;
                console.log(btn.index);
            }
        }

        Column {
            id: source
            SplitView.preferredWidth: root.cellWidth
            SplitView.maximumWidth: root.cellWidth * 2
            SplitView.minimumWidth: root.cellWidth

            Repeater {
                id: rep
                model: diagramModel
                delegate: Button {
                    id: btn
                    required property string name
                    required property string file
                    required property int index

                    implicitWidth: root.cellWidth
                    checkable: true
                    display: AbstractButton.TextUnderIcon

                    text: btn.name
                    icon.source: btn.file
                    icon.width: root.cellWidth * .75
                    icon.height: root.cellWidth * .4
                }
            }
        }

        //  Background used as canvas for object placement
        Item {
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
                property Diagram fromObject: null
                property Diagram toObject: null

                color: "transparent" //"gainsboro"

                //  Test Only Diagrams for checking line connection
                Diagram {
                    id: diagram1

                    Component.onCompleted: {
                        model.name = "AND Gate"
                        model.imageSource = "qrc:/and"
                        refresh(); // A hack, see Diagram.qml function for details
                    }
                    x: 100
                    y: 100
                    //z: 1
                }

                Diagram {
                    id: diagram2

                    Component.onCompleted: {
                        model.name = "OR Gate"
                        model.imageSource = "qrc:/or"
                        refresh(); // A hack, see Diagram.qml function for details
                    }

                    x: 200
                    y: 200
                    z: 1
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
                    visible: canvas.curIndex != -1
                }

                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: event => {
                        //  No template selected. Just return
                        if (canvas.curIndex === -1) {
                            return;
                        }

                        //  Find data
                        var item = diagramModel.get(parent.curIndex);
                        console.log("Index", parent.curIndex, "name", item.name, "image", item.file);

                        //  Create diagram
                        var diagram = Move.createBlock(canvas);

                        diagram.model.name = item.name;
                        diagram.model.imageSource = item.file;
                        diagram.model.type = parent.curIndex; //item.key;


                        //  Move object to final spot
                        Move.moveObjectTo(diagram, event.x, event.y);
                        diagram.refresh(); // A hack, see Diagram.qml function for details

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
