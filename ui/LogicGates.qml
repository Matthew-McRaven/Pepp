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
            file: "qrc:/and"
        }
        ListElement {
            name: "Inverter"
            type: "Inverter"
            file: "qrc:/and"
        }
        ListElement {
            name: "NAND Gate"
            type: "NAND"
            file: "qrc:/and"
        }
        ListElement {
            name: "NOR Gate"
            type: "NOR"
            file: "qrc:/and"
        }
        ListElement {
            name: "XOR Gate"
            type: "XOR"
            file: "qrc:/and"
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

        Rectangle {
            id: canvas

            /*Image{
                id: svgs
                anchors.fill: canvas
                source: "qrc:/logic_gates"
            }*/

            Rectangle {
                id: stamp
                color: "yellow"
                border {
                    color: "green"
                    width: 1
                }
                visible: root.curName != ""

                /*HoverHandler {
                    cursorShape: PointerDevice.Cursor
                    target: parent
                }*/
            }

            MouseArea {
                id: ma
                anchors.fill: parent
                onClicked: {
                    //  No template selected. Just return
                    if (root.curName === "")
                        return;

                    var comp = Qt.createComponent("Diagram.qml");
                    //const row = Math.floor((canvas.x + ma.mouseX) / root.blockSize) * root.blockSize;
                    //const col = Math.floor((canvas.y + ma.mouseY) / root.blockSize) * root.blockSize;

                    //var diagram = comp.createObject(root,{text: root.curName, x: row, y: col});
                    var diagram = comp.createObject(root, {
                        text: root.curName,
                        file: root.curFile
                        //x: row //canvas.x,
                        //y: col //canvas.y
                    });

                    //  Move object within grid (large axis)
                    Move.moveObjectTo(diagram, canvas.x + ma.mouseX, canvas.y + ma.mouseY);
                }
            }

            /*Component {
                id: tileComponent
                Tile{}
            }*/
        }
    }
}
