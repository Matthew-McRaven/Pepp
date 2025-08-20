pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property real topOffset: 0
    property list<int> filterEdition: []
    required property var font
    property real spacing: 0

    //  Required for accurate spacing in parent layout
    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width

    //  Custom control used for Flow below
    component EditionButton: Button {
        id: control
        required property int edition
        property real leftRadius: 0
        property real rightRadius: 0
        readonly property var p: enabled ? root.palette : root.palette.disabled
        down: settings.general.defaultEdition == control.edition
        enabled: root.filterEdition.length === 0 || root.filterEdition.includes(control.edition)
        font: root.font
        background: Rectangle {
            id: background
            color: control.enabled ? (control.down ? palette.highlight : (control.hovered ? palette.light : palette.button)) : palette.shadow
            topLeftRadius: control.leftRadius
            topRightRadius: control.rightRadius
            bottomLeftRadius: control.leftRadius
            bottomRightRadius: control.rightRadius
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.lighter(background.color, 1.1)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.darker(background.color, 1.1)
                }
            }
            border.color: Qt.darker(palette.button, 1.1)
            border.width: 1
        }
        onReleased: {
            settings.general.defaultEdition = control.edition;
        }
    }

    //  Edition selection string. Provide automatic line breaks if view is too narrow
    Flow {
        id: header
        spacing: root.spacing
        anchors {
            //  Do not set bottom equal to root, or layout will break
            left: root.left
            right: root.right
            top: root.top
        }

        Label {
            text: `Computer Systems`
            font: root.font
        }
        Row {
            spacing: -1
            EditionButton {
                text: "Sixth"
                edition: 6
                leftRadius: root.font.pointSize / 4
            }
            EditionButton {
                text: "Fifth"
                edition: 5
                rightRadius: settings.general.showDebugComponents ? 0 : root.font.pointSize / 4
            }
            EditionButton {
                visible: settings.general.showDebugComponents
                text: "Fourth"
                edition: 4
                rightRadius: root.font.pointSize / 4
            }
        }
        Label {
            Layout.fillWidth: true
            text: `Edition`
            font: root.font
        }
    }
}
