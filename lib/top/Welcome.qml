import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import edu.pepp 1.0

Item {
    id: root
    property real topOffset: 0
    property string loadingFileName: ""
    property var loadingFileContent: ""
    property list<int> filterEdition: []
    property list<int> filterAbstraction: []
    readonly property bool filtering: filterEdition.length !== 0 && filterAbstraction.length !== 0
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
            pointSize: 3 * fm.font.pointSize / 4
        }
    }
    FontMetrics {
        id: fnameFM
        font {
            family: fm.font.family
            pointSize: 3 * fm.font.pointSize / 4
        }
    }
    TextMetrics {
        id: projectTM
        font: projectFM.font
        text: "Level Asmb5"
    }

    signal addProject(int arch, int abstraction, string features, string optText, bool reuse)

    component EditionButton: Button {
        id: control
        required property int edition
        property real leftRadius: 0
        property real rightRadius: 0
        readonly property var p: enabled ? root.palette : root.palette.disabled
        down: settings.general.defaultEdition == control.edition
        enabled: root.filterEdition.length === 0 || root.filterEdition.includes(control.edition)
        font: fm.font
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

    RowLayout {
        id: filenameHeader
        spacing: fm.averageCharacterWidth
        visible: !!root.loadingFileName
        anchors {
            top: parent.top
            left: parent.left
            leftMargin: 10
            right: parent.right
            topMargin: visible ? -root.topOffset : 0
        }
        Label {
            text: `Loading from file:`
            font: fm.font
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            text: root.loadingFileName
            font: fnameFM.font
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        }
        Item {
            Layout.fillWidth: true
        }

        height: visible ? implicitHeight : 0
    }

    Flow {
        id: header
        spacing: fm.averageCharacterWidth
        anchors {
            top: filenameHeader.bottom
            left: parent.left
            leftMargin: 10
            right: parent.right
            topMargin: filenameHeader.visible ? 0 : -root.topOffset
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

    ScrollView {
        id: sv
        clip: true
        anchors {
            top: header.bottom
            topMargin: 20
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            margins: 10
        }
        GridLayout {
            id: list
            width: sv.width
            rowSpacing: 35
            columnSpacing: 5
            columns: Math.min(3, Math.max(2, sv.width / (projectTM.width * 1.1)))
            Repeater {
                model: projects
                delegate: RowLayout {
                    required property var model
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                    Layout.minimumWidth: projectTM.width * 1.1
                    Layout.maximumWidth: projectTM.width * 3.3
                    Layout.fillWidth: true
                    RoundButton {
                        id: button
                        Layout.fillWidth: true
                        font: projectFM.font
                        visible: !model.placeholder
                        text: model.text
                        onReleased: root.addProject(model.architecture, model.abstraction, "", root.loadingFileContent, false)
                        enabled: (model.complete || model.partiallyComplete) && (root.filterAbstraction.length === 0 || root.filterAbstraction.includes(model.abstraction))
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
    }

    ProjectTypeFilterModel {
        id: projects
        edition: settings.general.defaultEdition ?? 6
        showIncomplete: settings.general.showDebugComponents
        showPartiallyComplete: settings.general.showDebugComponents
        sourceModel: ProjectTypeModel {}
    }

    onFilterEditionChanged: {
        if (filterEdition === 0) {} else if (4 <= filterEdition && filterEdition <= 6) {
            settings.general.defaultEdition = filterEdition;
        }
    }
}
