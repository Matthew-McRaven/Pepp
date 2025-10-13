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
            model: root.itemModel
            delegate: Item {
                id: del
                required property int row
                required property int column
                required property int type
                required property bool active
                implicitHeight: arView.implicitHeight
                implicitWidth: arView.implicitWidth
                Component.onCompleted: {
                    computeScopeToIndex();
                }
                function computeScopeToIndex() {
                    const model = root.itemModel;
                    const idx = model.index(row, column);
                    scam.scopeToIndex = Qt.binding(() => idx);
                }
                ScopedActivationModel {
                    id: scam
                    sourceModel: root.itemModel
                }

                ActivationRecordView {
                    id: arView
                    font: root.font
                    active: del.active
                    implicitAddressWidth: root.implicitAddressWidth
                    implicitValueWidth: root.implicitValueWidth
                    implicitLineHeight: root.implicitLineHeight
                    boldBorderWidth: root.boldBorderWidth

                    lineModel: scam
                }
            }
        }
    }
}
