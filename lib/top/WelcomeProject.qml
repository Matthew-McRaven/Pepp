import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
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
    signal addProject(string arch, string abstraction, string features, bool reuse)
    signal goBack
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            spacing: 20
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
                text: `Choose a project for ${utils.archAsString(
                          settings.general.defaultArch)}`
                font: fm.font
            }
        }

        ListView {
            id: list
            model: projects
            Layout.fillWidth: true
            Layout.fillHeight: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            spacing: 20
            leftMargin: 20
            topMargin: 20
            bottomMargin: 20
            delegate: WelcomeCard {
                required property var model
                id: inner
                text: model.text
                architecture: model.architecture
                abstraction: model.abstraction
                enabled: model.complete || model.partiallyComplete
                source: model.source
                description: model.description
            }
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
