pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

//import CircuitDesign

Pane {
    id: root

    //  Model containing all components in current project
    //required property ComponentPropertyModel componentModel

    //  List of available blueprints for current project
    //required property BlueprintLibraryModel blueprintModel

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

        Grid {
            columns: 2
            spacing: 5
            Label {
                text: "Name:"
            }
            Text {
                text: "2 x 1"
            }
            Label {
                text: "Behavior:"
            }
            ComboBox {
                id: gateFamily
                model: ["AND", "OR", "NAND", "NOR", "XOR", "Inverter"]
                currentIndex: 0
            }
            Label {
                text: "Dimensions"
            }
            Label {
                //  Spacer for heading
                text: " "
            }

            Label {
                text: "  Height"
            }
            SpinBox {
                from: 2
                to: 10
                stepSize: 1
                value: 2
            }
            Label {
                text: "  Width"
            }
            SpinBox {
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
