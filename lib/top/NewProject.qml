pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    //  Grid layout
    property int columns: 4
    property real rowSpacing: 0
    property real columnSpacing: 0

    //  Grid cell layout
    property real cellRadius: 0
    property real cellWidth: 0
    required property var font

    //  Data
    property var loadingFileContent: ""
    property list<int> filterAbstraction: []
    required property var model

    signal addProject(int arch, int abstraction, string features, string optText, bool reuse)

    //  Layout of projects that may be picked by user.
    GridLayout {
        id: list

        width: root.width
        rowSpacing: root.rowSpacing
        columnSpacing: root.columnSpacing
        columns: root.columns
        Repeater {
            model: root.model
            delegate: RowLayout {
                required property var model
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                Layout.minimumWidth: root.cellWidth * 1.1
                Layout.maximumWidth: root.cellWidth * 3.3
                Layout.fillWidth: true
                RoundButton {
                    id: button
                    Layout.fillWidth: true
                    font: root.font
                    visible: !model.placeholder
                    text: model.text
                    onReleased: root.addProject(model.architecture, model.abstraction, "", root.loadingFileContent, false)
                    enabled: (model.complete || model.partiallyComplete) && (root.filterAbstraction.length === 0 || root.filterAbstraction.includes(model.abstraction))
                    palette.disabled.button: parent.palette.shadow
                    hoverEnabled: true
                    ToolTip.visible: (hovered || down) && model.description
                    ToolTip.delay: 500
                    ToolTip.text: qsTr(model.description)
                    radius: root.cellRadius
                }
                Item {
                    visible: model.placeholder
                    Layout.fillWidth: true
                }
            }
        }
        Item {
            id: spacer
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.columnSpan: list.columns
        }
    }
}
