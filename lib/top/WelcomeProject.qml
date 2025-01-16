import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
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
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            spacing: 20
            Layout.leftMargin: 10
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

        TableView {
            id: list
            model: projects
            Layout.fillWidth: true
            Layout.fillHeight: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            leftMargin: 10
            topMargin: 20
            bottomMargin: 20
            rowSpacing: 10
            delegate: DelegateChooser {
                role: "columnType"
                DelegateChoice {
                    roleValue: "name"
                    Button {
                        required property var model
                        text: model.text
                        font: fm.font
                        onReleased: {
                            root.addProject(model.architecture,
                                            model.abstraction, "", false)
                        }
                        enabled: model.complete || model.partiallyComplete
                        palette.disabled.button: parent.palette.shadow
                    }
                }
                DelegateChoice {
                    roleValue: "image"
                    Item {
                        required property var model
                        implicitWidth: 150
                        implicitHeight: 120
                        Image {
                            id: image
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            verticalAlignment: Image.AlignTop
                            horizontalAlignment: Image.AlignHCenter
                            source: model.source ? model.source : "image://icons/blank.svg"
                            clip: true
                        }
                    }
                }
                DelegateChoice {
                    roleValue: "description"
                    Text {
                        required property var model
                        text: model.description
                    }
                }
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
