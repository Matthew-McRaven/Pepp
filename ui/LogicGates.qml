pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage

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
            type: "key"
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
                component RoleData: QtObject {property string shapeType}
                function filter(data: RoleData) : bool {
                    return data.shapeType === "Diagram";
                }
            }
        ]
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

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        ButtonGroup {
            buttons: source.children.filter(child => child !== rep)

            onClicked: btn => {
                canvas.curName = btn.text;
                canvas.curFile = btn.icon.source;
                console.log(btn.text);
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
            //scale: .8
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

                property string curName: ""
                property string curFile: ""
                property string curType: ""
                property Diagram fromObject: null
                property Diagram toObject: null

                color: "transparent" //"gainsboro"


                //  Test Only Diagrams for checking line connection
                Diagram {
                    id: diagram1
                    //props: props
                    text: "AND Gate"
                    file: "qrc:/and"
                    x: 0
                    y: 0
                    z: 1
                }

                Diagram {
                    id: diagram2
                    text: "OR Gate"
                    file: "qrc:/or"
                    //props: props
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
                    visible: canvas.curName != ""
                }

                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: event => {
                        //  No template selected. Just return
                        if (canvas.curName === "")
                            return;

                        var diagram = Move.createBlock(canvas, event.x, event.y);
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
