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

            //  Indentation from edge
            leftMargin: root.horizontalMargins
            rightMargin: root.horizontalMargins
            topMargin: root.verticalMargins
            bottomMargin: root.verticalMargins
        }

        //  Colors
        color: palette.base
        border.color: palette.midlight
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: root.spacing
            anchors.topMargin: anchors.leftMargin

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

                //topOffset: root.topOffset
                filterEdition: root.filterEdition
                font: newFm.font

                Layout.fillWidth: true
            }   //  EditionSelector

            ScrollView {
                id: sv
                clip: true
                Layout.fillHeight: true
                Layout.fillWidth: true

                NewProject {
                    //  Grid layout
                    columns: Math.min(3, Math.max(2, sv.width / (projectTM.width * 1.1)))
                    rowSpacing: 35
                    columnSpacing: 5
                    width: sv.width

                    //  Cell layout
                    cellRadius: fm.font.pointSize / 2
                    cellWidth: projectTM.width
                    font: projectFM.font

                    //  Data
                    model: projects
                    loadingFileContent: root.loadingFileContent

                    onAddProject: function (arch, abs, feats, content, reuse) {
                        //  Bubble up event from child control
                        root.addProject(arch, abs, feats, content, reuse);
                    }

                }   //  NewProject
            }   //  ScrollView
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
