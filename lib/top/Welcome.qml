import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import edu.pepp 1.0

Item {
    id: root
    property real topOffset: 0
    NuAppSettings {
        id: settings
    }
    ArchitectureUtils {
        id: utils
    }

    FontMetrics {
        id: fm
        font.pointSize: 48
    }
    FontMetrics {
        id: projectFM
        font {
            family: fm.font.family
            pointSize: fm.font.pointSize * 2 / 3
        }
    }
    TextMetrics {
        id: projectTM
        font: projectFM.font
        text: "Level Asmb5"
    }

    signal addProject(int arch, int abstraction, string features, bool reuse)

    component EditionButton: Button {
        id: control
        required property int edition
        property real leftRadius: 0
        property real rightRadius: 0

        down: settings.general.defaultEdition == control.edition
        font: fm.font

        background: Rectangle {
            id: background
            color: control.down ? palette.highlight : (control.hovered ? palette.light : palette.button)
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

    Flow {
        id: header
        spacing: fm.averageCharacterWidth
        anchors {
            top: parent.top
            left: parent.left
            leftMargin: 10
            right: parent.right
            topMargin: -root.topOffset
        }
        Label {
            text: `Computer Systems`
            font: fm.font
        }
        Row {

            spacing: -1 // -border.width
            EditionButton {
                text: "Sixth"
                edition: 6
                leftRadius: fm.font.pointSize / 4
            }
            EditionButton {
                text: "Fifth"
                edition: 5
                rightRadius: settings.general.showDebugComponents ? 0 : fm.font.pointSize / 4
            }
            EditionButton {
                visible: settings.general.showDebugComponents
                text: "Fourth"
                edition: 4
                rightRadius: fm.font.pointSize / 4
            }
        }
        Label {
            Layout.fillWidth: true
            text: `Edition`
            font: fm.font
        }
    }

    GridLayout {
        id: list
        anchors {
            top: header.bottom
            topMargin: 20
            bottom: parent.bottom
            left: parent.left
            leftMargin: 10
            right: parent.right
            rightMargin: 10
        }

        clip: true
        rowSpacing: 55
        columnSpacing: 5
        columns: 3
        Repeater {
            model: projects
            delegate: RowLayout {
                required property var model
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                Layout.preferredWidth: list.width / list.columns
                Layout.minimumWidth: projectTM.width * 1.1
                RoundButton {
                    id: button
                    font: projectFM.font
                    visible: !model.placeholder
                    Layout.fillWidth: true
                    text: model.text
                    onReleased: {
                        root.addProject(model.architecture, model.abstraction, "", false);
                    }
                    enabled: model.complete || model.partiallyComplete
                    palette.disabled.button: parent.palette.shadow
                    hoverEnabled: true
                    ToolTip.visible: (hovered || down) && model.description
                    ToolTip.delay: 500
                    ToolTip.text: qsTr(model.description)
                    radius: fm.font.pointSize / 2
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
    ProjectTypeFilterModel {
        id: projects
        edition: settings.general.defaultEdition ?? 6
        showIncomplete: settings.general.showDebugComponents
        showPartiallyComplete: settings.general.showDebugComponents
        sourceModel: ProjectTypeModel {}
    }
}
