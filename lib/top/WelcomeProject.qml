import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import edu.pepp 1.0

Item {
    id: root
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
    signal addProject(int arch, int abstraction, string features, bool reuse)
    signal goBack
    RowLayout {
        id: header
        spacing: 20
        anchors {
            top: parent.top
            left: parent.left
            leftMargin: 10
            right: parent.right
        }

        Button {
            text: "Choose Different Architecture"
            onReleased: root.goBack()
            icon.source: "image://icons/navigation/arrow_back.svg"
            icon.height: fm.height
            icon.width: fm.height
            display: AbstractButton.TextBesideIcon
        }
        Label {
            Layout.fillWidth: true
            text: `Choose a project for ${utils.archAsString(settings.general.defaultArch)}`
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
                Button {
                    Layout.fillWidth: true
                    text: model.text
                    font: fm.font
                    onReleased: {
                        root.addProject(model.architecture, model.abstraction, "", false);
                    }
                    enabled: model.complete || model.partiallyComplete
                    palette.disabled.button: parent.palette.shadow
                    hoverEnabled: true
                    ToolTip.visible: (hovered || down) && model.description
                    ToolTip.delay: 500
                    ToolTip.text: qsTr(model.description)
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
        architecture: settings.general.defaultArch
        showIncomplete: settings.general.showDebugComponents
        showPartiallyComplete: settings.general.showDebugComponents
        sourceModel: ProjectTypeModel {}
    }
}
