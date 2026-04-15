pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import CircuitDesign

Pane {
    id: root

    //  Model containing all components in current project
    required property ComponentPropertyModel componentModel

    //  List of available blueprints for current project
    required property BlueprintLibraryModel blueprintModel

    Column {
        id: inputArea
        spacing: 2
        bottomPadding: 0

        //enabled: root.componentModel.component !== null ? true : false

        function updateInput() {
            //  Negative row indicates unitialized qindex
            if (root.componentModel == null)
                return;

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
                text: "Id:"
            }
            Label {
                id: id
                text: root.componentModel.id ?? " "
            }

            Label {
                text: "Family:"
            }
            ComboBox {
                id: gateFamily
                model: root.blueprintModel
                textRole: "name"
                valueRole: "id"
                currentValue: ""
                onActivated: {
                    root.blueprintModel.blueprint = gateFamily.currentValue;

                    console.log("Family", gateFamily.currentValue);
                    console.log("Types", root.blueprintModel.blueprintTypes);
                }
            }
            Label {
                text: "Type:"
            }
            ComboBox {
                id: gateType
                model: root.blueprintModel.blueprintTypes
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
                currentValue: root.componentModel.direction
            }
        }   //  Grid
        /*Row {
            Button {
                id: saveBtn
                text: "Save"
                width: 75

                onClicked: {
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
        }*/   //  Row
    }   //  Column
}   //  Panle
