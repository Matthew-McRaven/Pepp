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
            familyIcon: "qrc:/and"
            types: [
                ListElement {
                    typeName: "2x1"
                }
            ]
        }
        ListElement {
            family: "Or"
            familyIcon: "qrc:/or"
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
            familyIcon: "qrc:/inverter"
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
            familyIcon: "qrc:/nand"
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
            familyIcon: "qrc:/nor"
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
            height: 80
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
                            id: button
                            implicitHeight: 50
                            implicitWidth: 75
                            display: AbstractButton.TextUnderIcon

                            text: typeName
                            icon.source: familyIcon
                            icon.width: button.implicitWidth * .55
                            icon.height: button.implicitHeight * .55
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
