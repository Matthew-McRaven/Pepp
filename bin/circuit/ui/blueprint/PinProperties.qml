pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

//  Control for pin properties
Pane {
    id: root
    //spacing: 0

    property int labelWidth: 75
    property int dataWidth: 100

    Grid {
        columns: 2
        spacing: 5
        Label {
            text: "Pin Name:"
            width: root.labelWidth
        }
        Text {
            text: "Input 1"
            width: root.dataWidth
        }

        Label {
            text: "Type:"
            width: root.labelWidth
        }
        ComboBox {
            id: gateFamily
            width: root.dataWidth
            model: ["Input", "Output", "Bi-Directional", "Clock"]
            currentIndex: 2
        }

        Label {
            text: "Gate Location"
            width: root.labelWidth
        }
        Label {
            //  Spacer for heading
            text: " "
        }
        Label {
            text: "  x:"
            width: root.labelWidth
        }
        SpinBox {
            width: root.dataWidth / 2
            from: 1
            to: 20
            stepSize: 1
            value: 1
        }
        Label {
            text: "  y:"
            width: root.labelWidth
        }
        SpinBox {
            width: root.dataWidth / 2
            from: 1
            to: 10
            stepSize: 1
            value: 3
        }
    }   //  Grid
}   //  ColumnLayout
