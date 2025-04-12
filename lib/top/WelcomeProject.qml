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
            text: `Choose a project for ${utils.archAsString(
                      settings.general.defaultArch)}`
            font: fm.font
        }
    }

    TableView {
        id: list
        model: projects
        anchors {
            top: header.bottom
            topMargin: 20
            bottom: parent.bottom
            left: parent.left
            leftMargin: 10
            right: parent.right
            rightMargin: 10
        }

        boundsBehavior: Flickable.StopAtBounds
        clip: true
        rowSpacing: 10
        columnSpacing: 5
        contentWidth: width
        delegate: DelegateChooser {
            role: "columnType"
            DelegateChoice {
                roleValue: "name"
                Button {
                    required property var model
                    text: model.text
                    font: fm.font
                    onReleased: {
                        root.addProject(model.architecture, model.abstraction,
                                        "", false)
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
                Item {
                    // Use Item wrapper to prevent Text from setting implicitWidth/height poorly
                    required property var model
                    implicitWidth: list.width - list.columnWidth(
                                       0) - list.columnWidth(
                                       1) - 2 * list.columnSpacing
                    implicitHeight: desc.contentHeight
                    Text {
                        id: desc
                        anchors.fill: parent
                        text: model.description
                        wrapMode: Text.Wrap
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
