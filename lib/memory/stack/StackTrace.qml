import QtQuick 2.15
import edu.pepp 1.0

Rectangle {
    color: "orange"

    // Create C++ items using the magic of QQmlPropertyList and DefaultProperty
    /*ActivationModel {
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
    }*/

    ListModel {
        id: activationModel
        ListElement {
            active: false
            subItems: [
                ListElement {
                    address: 0
                    value: "10"
                    name: "a"
                },
                ListElement {
                    address: 2
                    value: "20"
                    name: "b"
                }
            ]
        }
        ListElement {
            active: true
            subItems: [
                ListElement {
                    address: 4
                    value: "30"
                    name: "c"
                },
                ListElement {
                    address: 12
                    value: "200"
                    name: "b2"
                }
            ]
        }
        ListElement {
            active: true
            subItems: [
                ListElement {
                    address: 6
                    value: "40"
                    name: "d"
                },
                ListElement {
                    address: 7
                    value: "50"
                    name: "e"
                },
                ListElement {
                    address: 9
                    value: "60"
                    name: "f"
                }
            ]
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

        //Column {
        MemoryStack {
            //Layout.alignment: Qt.AlignBottom
            //anchors.fill: parent
            //y: 100
            font: tm.font
            itemModel: activationModel
        }
        //}
    }


    /*ListView {
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
    }*/
}
