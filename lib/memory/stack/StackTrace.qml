import QtQuick 2.15
import edu.pepp 1.0

Rectangle {
    // Create C++ items using the magic of QQmlPropertyList and DefaultProperty
    ActivationModel {
        id: activationModel
        ActivationRecord {
            active: true
            RecordLine {
                address: 0
                value: "10"
                name: "a"
            }
            RecordLine {
                address: 2
                value: "20"
                name: "b"
            }
        }
        ActivationRecord {
            active: false
            RecordLine {
                address: 4
                value: "30"
                name: "c"
            }
        }
        ActivationRecord {
            active: true
            RecordLine {
                address: 6
                value: "40"
                name: "d"
            }
            RecordLine {
                address: 7
                value: "50"
                name: "e"
            }
            RecordLine {
                address: 9
                value: "60"
                name: "f"
            }
        }
    }

    //  Globals
    Rectangle {
        //width: 180
        //height: 200
        anchors.fill: parent
        anchors.topMargin: 8
        color: palette.base

        TextMetrics {
            id: tm
            font: Theme.font
            text: "W" // Dummy value to get width of widest character
        }

        //Column {
        MemoryStack {
            //y: 100
            font: tm.font
            itemModel: activationModel
        }
        //}
    }
}
