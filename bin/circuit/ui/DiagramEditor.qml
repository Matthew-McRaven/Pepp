pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import CircuitDesign
import DiagramEnum

Pane {
    id: root

    //  Model containing all diagrams
    required property DiagramDataModel diagramModel

    //  List of available gates
    required property FilterDiagramListModel gateModel

    Column {
        id: inputArea
        spacing: 2
        bottomPadding: 0

        enabled: inputArea.index.row !== -1

        property var index: root.diagramModel.currentIndex

        onIndexChanged: {
            //  Copies data from model to input areas
            inputArea.updateInput();
        }

        function updateInput() {
            //  Negative row indicates unitialized qindex
            if (root.diagramModel == null || inputArea.index.row === -1)
                return;

            //  Get data for current index
            id.text = root.diagramModel.data(inputArea.index, DiagramDataModel.Id);
            gateType.currentValue = root.diagramModel.data(inputArea.index, DiagramDataModel.Name);
            orientation.currentValue = root.diagramModel.data(inputArea.index, DiagramDataModel.Orientation);
            input.value = root.diagramModel.data(inputArea.index, DiagramDataModel.InputNo);
            output.value = root.diagramModel.data(inputArea.index, DiagramDataModel.OutputNo);
        }

        Grid {
            columns: 2
            spacing: 5
            Label {
                text: "Id:"
            }
            Label {
                id: id
                text: "  "
            }

            Label {
                text: "Gate Type:"
            }
            ComboBox {
                id: gateType
                model: root.gateModel
                textRole: "name"
                valueRole: "name"
                currentValue: ""
            }

            Label {
                text: "Orientation:"
            }
            ComboBox {
                id: orientation
                model: [
                    {
                        value: 0,
                        text: "Right"
                    },
                    {
                        value: 90,
                        text: "Bottom"
                    },
                    {
                        value: 180,
                        text: "Left"
                    },
                    {
                        value: 270,
                        text: "Top"
                    }
                ]
                textRole: "text"
                valueRole: "value"
                currentValue: 0
            }

            Label {
                text: "Input Number:"
            }
            SpinBox {
                id: input
                from: 1
                to: 8
                value: 2
            }

            Label {
                text: "Output Number:"
            }
            SpinBox {
                id: output
                from: 1
                to: 3
                value: 1
            }
        }   //  Grid
        Row {
            Button {
                id: saveBtn
                text: "Save"
                width: 75

                onClicked: {
                    //  If source data is bad, just return
                    //  Negative row indicates unitialized qindex
                    if (root.diagramModel == null || inputArea.index.row === -1)
                        return;

                    //  Update model with new data
                    var item = root.gateModel.diagramTemplate(gateType.currentIndex);

                    //  Get data for current index
                    const data = root.diagramModel.item(inputArea.index);

                    data.name = item.name;
                    data.imageSource = item.qrcFile;
                    data.type = item.key;
                    data.orientation = orientation.currentValue;
                    data.inputNo = input.value;
                    data.outputNo = output.value;

                    root.diagramModel.update(inputArea.index);
                }
            }
            Button {
                text: "Cancel"
                width: 75

                onClicked: {
                    //  Copies data from model to input areas
                    inputArea.updateInput();
                }
            }
        }   //  Row
    }   //  Column
}   //  Panle
