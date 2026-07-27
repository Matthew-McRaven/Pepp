pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

//  Control for pin properties
Pane {
    id: root

    property int labelWidth: 75
    property int dataWidth: 100

    Component.onCompleted: {
        t.selectAll();
    }

    Grid {
        columns: 2
        spacing: 5
        Label {
            text: "Pin Name:"
            width: root.labelWidth
        }
        Rectangle {
            color: "white"
            width: t.width
            height: t.height
            border {
                color: Palette.midlight
                width: 1
            }
            TextEdit {
                id: t
                text: "Input 1"
                width: root.dataWidth
            }
        }

        Label {
            text: "Type:"
            width: root.labelWidth
        }
        ComboBox {
            id: gateFamily
            width: root.dataWidth
            model: ["Normal", "Edge", "Invert"]
            //model: ["Input", "Output", "Bi-Directional", "Clock"]
            currentIndex: 0
        }
    }   //  Grid
}   //  ColumnLayout
