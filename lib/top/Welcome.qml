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

    //  File selector logic
    MouseArea { // Can't be inside filenameHeader with anchors.fill:parent because it is a layout.
        id: mouseArea
        anchors.fill: filenameHeader
        hoverEnabled: filenameHeader.visible
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
            text: settings.general.fileNameFor(root.loadingFileName)
            font: fnameFM.font
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            // Put tooltip near the text rather than the whole row. Tooltip placement can be bad on the row.
            ToolTip.visible: mouseArea.containsMouse && root.loadingFileName
            ToolTip.text: root.loadingFileName
            color: palette.link
        }
        Item {
            Layout.fillWidth: true
        }

        height: visible ? implicitHeight : 0
    }

    //  Control to select edition
    EditionSelector {
        id: header

        topOffset: root.topOffset
        filterEdition: root.filterEdition
        font: fm.font
        spacing: fm.averageCharacterWidth

        anchors {
            top: filenameHeader.bottom
            left: parent.left
            leftMargin: 10
            right: parent.right
            topMargin: filenameHeader.visible ? 0 : -root.topOffset
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

        }
    }   //  ScrollView

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
