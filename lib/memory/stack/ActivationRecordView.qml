import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property variant lineModel
    required property bool active
    // Helpers to "math" the position of the background rectangle.
    property double valueX: 0
    property double valueWidth: 0
    property double valueY: 0
    property double valueHeight: 0
    property alias font: tm.font
    implicitHeight: column.implicitHeight
    implicitWidth: column.implicitWidth

    TextMetrics {
        id: tm
        text: "W" // Dummy value to get width of widest character
    }
    // Sorry all, this bit is cursed.
    // Instead of trying to do the "right thing" and use three columns+repeaters and place a rectangle
    // around the center column inside the middle repeater, I'm opting to only use a single repeater.
    // This reducs the amount of synchronization code between the repeaters/columns, but it also means
    // we no longer obviously know where the middle column is.
    // So, as we create our rows, we record the location of the middle column.
    // We'll math out the location of the background rectangle using those values.
    Rectangle {
        id: background
        x: root.valueX - border.width / 2
        y: root.valueY - border.width / 2
        color: "transparent"
        width: root.valueWidth + border.width
        height: (root.lineModel.lines.length * root.valueHeight) + border.width
        border.color: root.active ? "black" : "transparent"
        border.width: 4
        z: 1
    }
    Column {
        id: column
        Repeater {
            model: root.lineModel.lines
            Row {
                spacing: 10
                Label {
                    width: 8 * tm.width
                    height: tm.height
                    font: tm.font
                    text: `0x${model.address.toString(16).padStart(
                              4, '0').toUpperCase()}`
                    horizontalAlignment: Text.AlignRight
                }
                Rectangle {
                    id: valueRect
                    width: 8 * tm.width
                    height: tm.height + 2
                    border.color: "black"
                    border.width: 1
                    Label {
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: tm.font
                        text: model.value
                    }
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    width: 8 * tm.width
                    height: tm.height
                    font: tm.font
                    text: model.name
                }
                Component.onCompleted: {
                    // Record position of the value column so we can place the background rect.
                    if (model.row == 0) {
                        root.valueX = Qt.binding(function () {
                            return valueRect.x
                        })
                        root.valueWidth = Qt.binding(function () {
                            return valueRect.width
                        })
                        root.valueY = Qt.binding(function () {
                            return valueRect.y
                        })
                        root.valueHeight = Qt.binding(function () {
                            return valueRect.height
                        })
                    }
                }
            }
        }
    }
}
