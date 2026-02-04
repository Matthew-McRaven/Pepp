pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import DiagramEnum

Pane {
    id: root
    required property var diagramModel
    property var currentIndex: null

    //  List of available gates
    required property var gateModel

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
                model: root.gateModel
                textRole: "name"
                valueRole: "name"
                currentValue: root.currentIndex == null ?
                    "" : root.diagramModel.name
            }

            Label {
                text: "Input Number: "
            }
            SpinBox {
                id: input
                from: 1
                to: 6
                value: 2 //root.currentIndex ? root.diagramModel.inputNo : 2
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
