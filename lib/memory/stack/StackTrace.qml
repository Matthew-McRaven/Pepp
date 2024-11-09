import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {

    color: palette.base
    TextMetrics {
        id: tm
        font: Theme.font
        text: "W" // Dummy value to get width of widest character
    }
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

    ScrollView {
        anchors.fill: parent
        anchors.topMargin: 8
        contentWidth: column.width // The important part
        contentHeight: column.height // Same
        clip: true // Prevent drawing column outside the scrollview borders

        ColumnLayout {
            id: column
            MemoryStack {
                //y: 100
                id: globals
                font: tm.font
                itemModel: activationModel
            }
            Item {
                id: globalHeapBreak
                implicitHeight: 40
            }
            MemoryStack {
                id: heap
                font: tm.font
                itemModel: activationModel
            }
            Item {
                Layout.fillHeight: true
                implicitHeight: 80
            }
            MemoryStack {
                id: stack
                font: tm.font
                itemModel: activationModel
            }
        }
    }
}
