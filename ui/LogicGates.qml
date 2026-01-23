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

    ListModel {
        id: diagramModel
        ListElement {
            key: DiagramProperty.Invalid
            shapeType: "Move"
            name: "Move"
            file: "qrc:/move"
        }
        ListElement {
            key: DiagramProperty.ANDGate
            shapeType: "Diagram"
            name: "AND Gate"
            file: "qrc:/and"
        }
        ListElement {
            key: DiagramProperty.ORGate
            shapeType: "Diagram"
            name: "OR Gate"
            file: "qrc:/or"
        }
        ListElement {
            key: DiagramProperty.Inverter
            shapeType: "Diagram"
            name: "Inverter"
            file: "qrc:/inverter"
        }
        ListElement {
            key: DiagramProperty.NANDGate
            shapeType: "Diagram"
            name: "NAND Gate"
            file: "qrc:/nand"
        }
        ListElement {
            key: DiagramProperty.NORGate
            shapeType: "Diagram"
            name: "NOR Gate"
            file: "qrc:/nor"
        }
        ListElement {
            key: DiagramProperty.XORGate
            shapeType: "Diagram"
            name: "XOR Gate"
            file: "qrc:/xor"
        }
        ListElement {
            key: DiagramProperty.Line
            shapeType: "line"
            name: "Line"
            file: "qrc:/line"
        }
        ListElement {
            key: DiagramProperty.MultiLine
            shapeType: "Line"
            name: "Multiline"
            file: "qrc:/multiline"
        }
        ListElement {
            key: DiagramProperty.Bus
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

            ButtonGroup {
                buttons: source.children.filter(child => child !== rep)

                onClicked: btn => {
                    canvas.curIndex = btn.index;
                    console.log(btn.index);
                }
            }

            GridLayout {
                id: source
                columns: 2
                columnSpacing: 2
                rowSpacing: 2

                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true

                Repeater {
                    id: rep
                    model: diagramModel
                    delegate: Button {
                        id: btn
                        required property string name
                        required property string file
                        required property int index

                        implicitWidth: root.cellWidth
                        implicitHeight: root.cellWidth * .6
                        checkable: true
                        display: AbstractButton.TextUnderIcon

                        text: btn.name
                        icon.source: btn.file
                        icon.width: btn.implicitWidth * .8
                        icon.height: btn.implicitHeight * .7
                    }
                }
            }   //  GridLayout

            Item {  //  A spacer
                Layout.fillHeight: true
            }

            DiagramProperties {
                id: props
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: true
                //height: 100

                diagramModel: diagramModel.currentDiagram
                model: filterModel
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
                    }
                    x: 100
                    y: 100
                    z: 1
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
                        diagram.model.type = item.key;


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
