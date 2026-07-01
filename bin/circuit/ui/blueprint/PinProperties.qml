pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

//  Control for pin properties
ColumnLayout {
    id: root
    spacing: 0

    Grid {
        columns: 2
        spacing: 5
        Label {
            text: "Pin Properties"
        }
        Label {
            //  Spacer for heading
            text: " "
        }

        Label {
            text: "Pin Name:"
        }
        Text {
            text: "Input 1"
        }

        Label {
            text: "Type:"
        }
        ComboBox {
            id: gateFamily
            model: ["Input", "Output", "Bi-Directional", "Clock"]
            currentIndex: 0
        }

        Label {
            text: "x Location"
        }
        SpinBox {
            from: 1
            to: 20
            stepSize: 1
            value: 1
        }
        Label {
            text: "y Location"
        }
        SpinBox {
            from: 1
            to: 10
            stepSize: 1
            value: 3
        }
        Item {
            //  Spacer for heading
        }
    }   //  Grid

    Item {
        //  A spacer
        Layout.fillHeight: true
    }
}   //  ColumnLayout
