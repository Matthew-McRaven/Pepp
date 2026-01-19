pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Pane {
    id: root
    property var diagram: null

    //  List of available gates
    required property var model

    //enabled: diagram === null
    Grid {
        columns: 2
        spacing: 5
        Label {
            text: "Gate Type: "
        }
        ComboBox {
            model: root.model
            textRole: "name"
            valueRole: "key"
            //currentValue: root.diagram.text === null ? "" : root.diagram.text

            /*Component.onCompleted: {
                console.log("Diagram name: ", root.diagram.name);
                console.log("Diagram type: ", root.diagram.type);
            }*/
        }

        Label {
            text: "Input Number: "
        }
        SpinBox {
            from: 1
            to: 6
            value: 2
        }

        Label {
            text: "Output Number: "
        }
        SpinBox {
            from: 1
            to: 3
            value: 1
        }
    }
}
