import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property int stateChange: 0
    required property variant itemModel
    property alias font: tm.font
    implicitHeight: column.childrenRect.height

    TextMetrics {
        id: tm
        text: "W" // Dummy value to get width of widest character
    }
    Column {
        id: column
        Repeater {
            id: repeater
            model: root.itemModel.records
            ActivationRecordView {
                required property variant model
                font: tm.font
                lineModel: model
                active: model.active
            }
        }
    }
}
