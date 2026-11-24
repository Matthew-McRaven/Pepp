pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Pane {
    id: root

    //  Model containing all components in current project
    //required property ComponentPropertyModel componentModel

    //  List of available blueprints for current project
    //required property BlueprintLibraryModel blueprintModel

    property int labelWidth: 75
    property int dataWidth: 100

    Column {
        id: inputArea
        spacing: 2
        bottomPadding: 0

        //enabled: root.componentModel.component !== null ? true : false

        function updateInput() {
            //  Negative row indicates unitialized qindex
            //if (root.componentModel == null)
            //    return;

            //  Get data for current index
            /*id.text = root.diagramModel.data(inputArea.index, DiagramDataModel.Id);
            gateType.currentValue = root.diagramModel.data(inputArea.index, DiagramDataModel.Name);
            orientation.currentValue = root.diagramModel.data(inputArea.index, DiagramDataModel.Orientation);
            input.value = root.diagramModel.data(inputArea.index, DiagramDataModel.InputNo);
            output.value = root.diagramModel.data(inputArea.index, DiagramDataModel.OutputNo);*/
        }

        Component.onCompleted: {
            t.selectAll();
        }

        Grid {
            columns: 2
            spacing: 5
            Label {
                text: "Name:"
                width: root.labelWidth
            }
            Rectangle {
                color: "white"
                width: t.width
                height: t.height
                border {
                    color: "#a9a9a9"
                    width: 1
                }
                TextEdit {
                    id: t
                    text: "2 x 1"
                    width: root.dataWidth
                }
            }
            Label {
                text: "Behavior:"
                width: root.labelWidth
            }
            ComboBox {
                id: gateFamily
                model: ["AND", "OR", "NAND", "NOR", "XOR", "Inverter", "Custom"]
                currentIndex: 0
                width: root.dataWidth
            }
            Label {
                text: "Image:"
                width: root.labelWidth
            }
            ComboBox {
                id: imageFamily
                model: ["AND", "OR", "NAND", "NOR", "XOR", "Inverter"]
                currentIndex: 0
                width: root.dataWidth
                enabled: gateFamily.displayText == "Custom"
            }
            Label {
                text: "Dimensions"
                width: root.labelWidth
            }
            Label {
                //  Spacer for heading
                text: " "
            }

            Label {
                text: "  Height"
                width: root.labelWidth
            }
            SpinBox {
                width: root.dataWidth / 2
                from: 2
                to: 10
                stepSize: 1
                value: 2
            }
            Label {
                text: "  Width"
                width: root.labelWidth
            }
            SpinBox {
                width: root.dataWidth / 2
                from: 3
                to: 10
                stepSize: 1
                value: 3
            }
            Label {
                //  Spacer between buttons
                text: "  "
            }
        }   //  Grid
        //  Button actions
        Row {
            Button {
                id: copyBtn
                text: "Copy"
                width: 75

                /*onClicked: {
                    //  If source data is bad, just return
                    //  Negative row indicates unitialized qindex
                    if (root.diagramModel == null || inputArea.index.row === -1)
                        return;

                    var item = root.gateModel.diagramTemplate(gateType.currentIndex);

                    root.diagramModel.setData(inputArea.index, item.name, DiagramDataModel.Name);
                    root.diagramModel.setData(inputArea.index, item.qrcFile, DiagramDataModel.ImageSource);
                    root.diagramModel.setData(inputArea.index, item.key, DiagramDataModel.DiagramType);
                    root.diagramModel.setData(inputArea.index, orientation.currentValue, DiagramDataModel.Orientation);
                    root.diagramModel.setData(inputArea.index, input.value, DiagramDataModel.InputNo);
                    root.diagramModel.setData(inputArea.index, output.value, DiagramDataModel.OutputNo);
                }*/
            }
            Button {
                text: "Delete"
                width: 75

                /*onClicked: {
                    //  Copies data from model to input areas
                    inputArea.updateInput();
                }*/
            }
        }   //  Row
    }   //  Column
}   //  Panle
