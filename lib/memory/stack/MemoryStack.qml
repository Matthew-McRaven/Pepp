import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property int stateChange: 0
    required property variant itemModel

    //  Trigger height of this control based on height of all children
    implicitHeight: column.childrenRect.height

    //  Pass in dimensions & font
    required property variant font
    required property double implicitAddressWidth
    required property double implicitValueWidth
    required property double implicitLineHeight
    required property double boldBorderWidth

    Column {
        id: column
        Repeater {
            id: repeater
            model: root.itemModel.records
            ActivationRecordView {
                required property variant model
                font: root.font
                implicitAddressWidth: root.implicitAddressWidth
                implicitValueWidth: root.implicitValueWidth
                implicitLineHeight: root.implicitLineHeight
                boldBorderWidth: root.boldBorderWidth

                lineModel: model
                active: model.active
            }
        }
    }
}
