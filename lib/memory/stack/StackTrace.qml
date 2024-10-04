import QtQuick 2.15
import edu.pepp 1.0

Rectangle {
    color: "orange"
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

    Column {
        anchors.fill: parent
        Repeater {
            model: activationModel.records
            delegate: Column {
                id: recordDelegate
                required property var modelData
                Repeater {
                    model: modelData.lines
                    delegate: Text {
                        required property var modelData
                        text: `${modelData.address} -- ${modelData.value} -- ${modelData.name}`
                        font.bold: recordDelegate.modelData.active
                    }
                }
            }
        }
    }
}
