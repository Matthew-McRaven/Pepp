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
            name: "AND Gate"
            type: "AND"
            file: "qrc:/and"
        }
        ListElement {
            name: "OR Gate"
            type: "OR"
            file: "qrc:/or"
        }
        ListElement {
            name: "Inverter"
            type: "Inverter"
            file: "qrc:/inverter"
        }
        ListElement {
            name: "NAND Gate"
            type: "NAND"
            file: "qrc:/nand"
        }
        ListElement {
            name: "NOR Gate"
            type: "NOR"
            file: "qrc:/nor"
        }
        ListElement {
            name: "XOR Gate"
            type: "XOR"
            file: "qrc:/xor"
        }
        ListElement {
            name: "Line"
            type: "Line"
            file: "qrc:/line"
        }
        ListElement {
            name: "Multiline"
            type: "Multiline"
            file: "qrc:/multiline"
        }
        ListElement {
            name: "Bus"
            type: "Bus"
            file: "qrc:/bus"
        }
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
        Rectangle {
            id: canvas

            property string curName: ""
            property string curFile: ""
            property Diagram fromObject: null
            property Diagram toObject: null

            /*Image{
                id: svgs
                anchors.fill: canvas
                source: "qrc:/logic_gates"
            }*/

            //  Test Only Diagrams for checking line connection
            /*Diagram {
                id: diagram1
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
                x: 200
                y: 200
                z: 1
            }

            Line {
                id: line

                fromObject: diagram1
                toObject: diagram2
            }*/

            //  Used to show where objects will be stamped on canvas
            Rectangle {
                id: stamp
                color: "transparent"
                border {
                    color: "blue"
                    width: 1
                }
                width: 75
                height: 75
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
        }
    }
}
