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

    //  Globals
    Rectangle {
        //width: 180
        //height: 200
        anchors.fill: parent
        color: palette.base

        TextMetrics {
            id: tm
            font: Theme.font
            text: "W" // Dummy value to get width of widest character
        }

        ListView {
            anchors.fill: parent
            spacing: 0
            //  Using example 5.22 for sample global
            model: ListModel {

                //active: true
                ListElement {
                    address: 3
                    value: "M"
                    name: "ch"
                    action: 0
                }
                ListElement {
                    address: 4
                    value: "419"
                    name: "j"
                    action: 1
                }
            }

            header: StackItem {
                isHeader: true
                heading: "Address"
                value: "Value"
                name: "Name"
                z: 2 //  Make sure header is on top of children
                charWidth: tm.width
                implicitWidth: tm.width * 24
                implicitHeight: tm.height
            }
            delegate: StackItem {
                address: model.address
                value: model.value
                name: model.name
                stateChange: model.action
                charWidth: tm.width
                implicitWidth: tm.width * 24
                implicitHeight: tm.height
            }

            //focus: true
        }
    }


    /*
            text: `0x${root.currentAddress.toString(16).padStart(
                      4, '0').toUpperCase()}`
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
    }*/
}
