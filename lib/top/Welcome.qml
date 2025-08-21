import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import edu.pepp 1.0

Item {
    id: root
    property real topOffset: 0
    property real horizontalMargins: 10
    property real verticalMargins: 0
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

    //  Font metrics for new layout
    FontMetrics {
        id: newFm
        //font.pointSize: 48
    }
    TextMetrics {
        id: newTM
        font: newFm.font
        //font.pointSize: newFm.font.pointSize * 1.2
        text: "Bare Metal"
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

    Rectangle {
        id: background

        //  Layout
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        //  Colors
        color: palette.base
        border.color: palette.midlight
        border.width: 1

        //  Welcome screen is wrapped in column layout to assist with alignment
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: root.horizontalMargins
            anchors.rightMargin: root.horizontalMargins
            anchors.topMargin: root.verticalMargins
            anchors.bottomMargin: root.verticalMargins

            spacing: 5

            RowLayout {
                id: filenameHeader
                spacing: fm.averageCharacterWidth
                visible: !!root.loadingFileName
                Layout.fillWidth: true
                Layout.minimumHeight: filenameHeader.visible ? implicitHeight : 0
                Layout.maximumHeight: Layout.minimumHeight


                Label {
                    text: `Loading from file:`
                    font: fm.font
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    id: fileName
                    text: settings.general.fileNameFor(root.loadingFileName)
                    font: fnameFM.font
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    // Put tooltip near the text rather than the whole row. Tooltip placement can be bad on the row.
                    ToolTip.visible: mouseArea.containsMouse && root.loadingFileName
                    ToolTip.text: root.loadingFileName
                    color: palette.link

                    //  Can't be inside filenameHeader with anchors.fill:parent
                    //  because it is a layout. Nest in Text instead.
                    MouseArea {
                        id: mouseArea
                        anchors.fill: fileName
                        hoverEnabled: filenameHeader.visible
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
            }   //  RowLayout

            //  Control to select edition
            EditionSelector {
                id: header

                filterEdition: root.filterEdition
                font: newFm.font

                Layout.fillWidth: true
            }   //  EditionSelector

            Label {
                text: "New Projects"
                Layout.topMargin: 5
            }

            NewProject {
                id: project
                clip: true

                //  layout
                Layout.fillWidth: true
                spacing: 10

                //  Cell layout
                cellRadius: 5
                cellWidth: newTM.width * 2
                cellHeight: newTM.height * 5
                font: newTM.font

                //  Data
                model: projects
                loadingFileContent: root.loadingFileContent

                onAddProject: function (arch, abs, feats, content, reuse) {
                    //  Bubble up event from child control
                    root.addProject(arch, abs, feats, content, reuse);
                }
            }   //  NewProject
            Item {
                id: spacer
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }   //  ColumnLayout
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
