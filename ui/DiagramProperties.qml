pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Pane {
    id: root
    required property Diagram diagram
    required property var model

    Grid {
        columns: 2
        spacing: 5
        Label {
            text: "Gate Type: "
        }
        ComboBox {
            model: root.model
            textRole: "type"
            valueRole: "type"
            currentValue: root.diagram.type

            Component.onCompleted: console.log(root.diagram.type)
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
