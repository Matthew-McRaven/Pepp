pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "move.js" as Move

Rectangle {
    id: root

    property real cellWidth: 128
    //property int blockSize: 100
    property string curName: ""
    property string curFile: ""

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
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        ButtonGroup {
            buttons: source.children.filter(child => child !== rep)

            onClicked: btn => {
                root.curName = btn.text;
                root.curFile = btn.icon.source;
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

            /*Image{
                id: svgs
                anchors.fill: canvas
                source: "qrc:/logic_gates"
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
                visible: root.curName != ""
            }


            MouseArea {
                id: ma
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    //  No template selected. Just return
                    if (root.curName === "")
                        return;

                    var comp = Qt.createComponent("Diagram.qml");

                    var diagram = comp.createObject(root, {
                                                        text: root.curName,
                                                        file: root.curFile
                                                    });

                    //  Move object within grid (large axis)
                    diagram.x = canvas.x + stamp.x;
                    diagram.y = canvas.y + stamp.y;
                }

                onPositionChanged: event => {
                    //  Move object within grid (large axis)
                    Move.moveObjectTo(stamp, event.x, event.y);
                    //console.log( "x", event.x, "y", event.y, "new x", row, "new y", col);
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
