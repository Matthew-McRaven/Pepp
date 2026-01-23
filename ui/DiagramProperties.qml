pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Pane {
    id: root
    property var diagramModel: null

    //  List of available gates
    required property var model
    Column {
        spacing: 2
        bottomPadding: 0

        Grid {
            columns: 2
            spacing: 5
            Label {
                text: "Gate Type: "
            }
            ComboBox {
                id: gateType
                model: root.model
                textRole: "name"
                valueRole: "name"
                currentValue: root.diagramModel.name === null ? "" : root.diagramModel.name

                /*Component.onCompleted: {
                    console.log("Diagram name: ", root.diagram.name);
                    console.log("Diagram type: ", root.diagram.type);
                }*/
            }

            Label {
                text: "Input Number: "
            }
            SpinBox {
                id: input
                from: 1
                to: 6
                value: 2
            }

            Label {
                text: "Output Number: "
            }
            SpinBox {
                id: output
                from: 1
                to: 3
                value: 1
            }
        }   //  Grid
        Row {
            //height: saveBtn.implicitHeight
            //width: root.width
            Button {
                id: saveBtn
                text: "Save"
                width: 75

                onClicked: {
                    //  If source data is bad, just return
                    if (gateType.currentIndex < 0 || root.diagramModel === null) {
                        return;
                    }

                    //  Update model with new data
                    //  Extra item in model versus filter model. Find fix instead of
                    //  hard coding value.
                    var item = root.model.sourceModel.get(gateType.currentIndex + 1);
                    root.diagramModel.name = item.name;
                    root.diagramModel.imageSource = item.file;
                    root.diagramModel.type = item.key;
                    root.diagramModel.inputNo = input.value;
                    root.diagramModel.outputNo = output.value;
                }
            }
            Button {
                text: "Cancel"
                width: 75

                onClicked: {
                    //  If source data is bad, just return
                    if (root.diagramModel === null) {
                        return;
                    }

                    //  Reset screen with model data
                    //gateType.currentText = root.diagramModel.name;
                }
            }
        }   //  Row
    }   //  Column
}   //  Panle
