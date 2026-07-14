//pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root

    ListModel {
        id: gatesList
        ListElement {
            family: "And"
            types: [
                ListElement {
                    typeName: "2x1"
                }
            ]
        }
        ListElement {
            family: "Or"
            types: [
                ListElement {
                    typeName: "2x1"
                },
                ListElement {
                    typeName: "3x1"
                }
            ]
        }
        ListElement {
            family: "Inverter"
            types: [
                ListElement {
                    typeName: "2x1"
                },
                ListElement {
                    typeName: "4x1"
                }
            ]
        }
        ListElement {
            family: "Nand"
            types: [
                ListElement {
                    typeName: "2x1"
                },
                ListElement {
                    typeName: "3x1"
                },
                ListElement {
                    typeName: "4x1"
                }
            ]
        }
        ListElement {
            family: "Nor"
            types: [
                ListElement {
                    typeName: "2x1"
                },
                ListElement {
                    typeName: "5x1"
                }
            ]
        }
    }

    Component {
        id: displayDelegate
        Rectangle {
            id: rowID
            width: 300
            height: 40
            Row {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                Text {
                    id: nameID
                    text: family
                    font.pixelSize: 12
                    width: 50
                    wrapMode: Text.WrapAnywhere
                }

                Repeater {
                    model: types
                    Text {
                        text: typeName + "\t"
                    }
                }
            }
        }
    }

    ListView {
        id: disp
        anchors.fill: parent
        model: gatesList
        delegate: displayDelegate
    }
}
