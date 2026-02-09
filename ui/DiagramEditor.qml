pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import CircuitDesign
import DiagramEnum

Pane {
    id: root
    required property DiagramDataModel diagramModel

    //  List of available gates
    required property var gateModel

    Column {
        id: col
        spacing: 2
        bottomPadding: 0

        enabled: col.index.row != -1

        property var index: root.diagramModel.currentIndex

        onIndexChanged: {
            console.log(root.diagramModel);
            const index = root.diagramModel.currentIndex;

            //  Negative row indicates unitialized qindex
            if(index.row == -1)
                return;

            //  Get data for current index
            const item = root.diagramModel.item(index);
            console.log(item);

            gateType.currentValue = item.name;
        }

        Grid {
            columns: 2
            spacing: 5
            Label {
                text: "Gate Type: "
            }
            ComboBox {
                id: gateType
                model: root.gateModel
                textRole: "name"
                valueRole: "name"
                currentValue: ""
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
                value: 1 //root.currentIndex ? root.diagramModel.outputNo : 1
            }
        }   //  Grid
        Row {
            Button {
                id: saveBtn
                text: "Save"
                width: 75

                onClicked: {
                    //  If source data is bad, just return
                    if (gateType.currentIndex < 0 || root.currentIndex == null) {
                        return;
                    }

                    //  Update model with new data
                    //  Extra item in model versus filter model. Find fix instead of
                    //  hard coding value.
                    var item = root.gateModel.sourceModel.diagramTemplate(gateType.currentIndex + 1);

                    var data = root.diagramModel.itemData(root.currentIndex);
                    data.name = item.name;
                    data.imageSource = item.qrcFile;
                    data.type = item.key;
                    data.inputNo = input.value;
                    data.outputNo = output.value;

                    const row = root.currentIndex.row;
                    const col = root.currentIndex.column;
                    root.diagramModel.update(row, col);
                }
            }
            Button {
                text: "Cancel"
                width: 75

                onClicked: {
                    //  If source data is bad, just return
                    if (root.diagramModel == null || root.currentIndex == null) {
                        return;
                    }

                    //  Reset screen with model data
                    var data = root.diagramModel.itemData(root.currentIndex);
                    console.log("data.name", data.name)
                    gateType.currentValue = data.name;
                    input.value = data.inputNo;
                    output.value = data.outputNo;
                }
            }
        }   //  Row
    }   //  Column
}   //  Panle
