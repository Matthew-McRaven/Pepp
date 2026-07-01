pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

ColumnLayout {
    id: root
    spacing: 0

    Image {
        id: image

        Layout.fillWidth: true
        source: "qrc:///gatelist"
        fillMode: Image.PreserveAspectFit
    }

    Grid {
        columns: 2
        spacing: 5
        Label {
            text: "Dimensions"
        }
        Label {
            //  Spacer for heading
            text: " "
        }

        Label {
            text: "Family:"
        }
        ComboBox {
            id: gateFamily
            model: ["AND", "OR", "NAND", "NOR", "XOR", "Inverter"]
            currentIndex: 0
        }
        Label {
            text: "Type:"
        }
        ComboBox {
            id: gateType
            model: ["OR 2x1"]
            currentIndex: 0
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
            currentIndex: 0
        }
        Label {
            text: "Height"
        }
        SpinBox {
            from: 2
            to: 10
            stepSize: 1
            value: 2
        }
        Label {
            text: "Width"
        }
        SpinBox {
            from: 3
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
