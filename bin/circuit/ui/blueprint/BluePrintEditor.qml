//pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root

    //  Temporary model for laying out screen
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
            width: 300
            height: 50
            Column {
                id: gateParent
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

                //ListView {
                //orientation: ListView.Horizontal
                Row {
                    Repeater {
                        model: types
                        Button {
                            implicitHeight: 25
                            implicitWidth: 75
                            text: typeName
                        }
                    }
                }   //  Row
            }   //  Column
        }   //  Rectangle
    }   //  Component

    ListView {
        anchors.fill: parent
        model: gatesList
        delegate: displayDelegate
    }
}
