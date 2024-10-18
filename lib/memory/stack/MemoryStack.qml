import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    //property bool isHeader: false
    property int stateChange: 0
    //property int address: 0 //  Used by stack records
    //property int charWidth: 8
    required property variant itemModel
    property alias font: tm.font

    //anchors.fill: parent
    Component.onCompleted: {
        console.log(itemModel)
    }
    TextMetrics {
        id: tm
        text: "W" // Dummy value to get width of widest character
    }

    Row {
        anchors.fill: parent

        Column {
            spacing: 0
            Repeater {
                model: root.itemModel
                Item {
                    width: addrCol2.width + 4
                    height: addrCol2.childrenRect.height + 2
                    Column {
                        id: addrCol2
                        property variant subItemModel: model.subItems
                        anchors.centerIn: parent
                        Repeater {
                            //id: repeater2
                            model: parent.subItemModel

                            Label {
                                width: 8 * tm.width
                                height: tm.height
                                font: tm.font
                                text: `0x${model.address.toString(16).padStart(
                                          4, '0').toUpperCase()}`
                            }
                        }
                    }
                }
            }
        } //  Column id: addrCol
        Column {
            spacing: 0
            Repeater {
                model: root.itemModel
                Item {
                    width: valCol2.width + 4
                    height: valCol2.childrenRect.height + 2
                    Rectangle {
                        anchors.leftMargin: 1
                        anchors.topMargin: 1

                        border.color: index === 0 ? "tranparent" : "black"
                        border.width: 1
                        height: tm.height * model.subItems.count + 2
                        width: 8 * tm.width + 4
                        Column {
                            id: valCol2
                            property variant subItemModel: model.subItems
                            anchors.centerIn: parent
                            Repeater {
                                model: parent.subItemModel

                                Rectangle {
                                    id: borderRect
                                    border.color: "black"
                                    border.width: 1
                                    width: 8 * tm.width + 2 //  Margin around text
                                    height: valLabel.height
                                    Label {
                                        id: valLabel
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        font: tm.font
                                        text: model.value
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } //  Column id: valCol
        Column {
            //  Name Column
            spacing: 0
            Repeater {
                model: root.itemModel
                Item {
                    width: nameCol2.width + 4
                    height: nameCol2.childrenRect.height + 2
                    Column {
                        id: nameCol2
                        property variant subItemModel: model.subItems
                        anchors.centerIn: parent
                        Repeater {
                            //id: repeater2
                            model: parent.subItemModel

                            Label {
                                width: 8 * tm.width
                                height: tm.height
                                font: tm.font
                                text: model.name
                            }
                        }
                    }
                }
            }
        } //  Column id: nameCol
    } //  Row
}
