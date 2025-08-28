pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    //  Control layout
    property real spacing: 0

    //  Cell layout
    property real cellRadius: 0
    property real cellWidth: 0
    property real cellHeight: 0
    required property var font

    //  Data
    property var loadingFileContent: ""
    property list<int> filterAbstraction: []
    required property var model

    signal addProject(int arch, int abstraction, string features, string optText, bool reuse)

    //  Layout does not work without implicit height and width
    implicitHeight: Math.min(layout.height, (root.cellHeight + spacing) * Math.min(2, layout.rowCnt))
    implicitWidth: Math.max(layout.width, (root.cellWidth + spacing) * layout.columns) + spacing

    //  Component shown in each cell of gridview
    component ProjectButton: Item {
        id: wr
        //  Declare required properties
        required property int index
        required property var model

        implicitHeight: root.cellHeight
        implicitWidth: root.cellWidth

        //  Button outline on hover
        Rectangle {
            anchors.fill: wr
            radius: root.cellRadius
            visible: (btn.hovered && wr.model.complete)
            border.width: 2
            border.color: palette.accent
            color: "transparent"
            z: 1
        }

        RoundButton {
            id: btn

            anchors.fill: wr
            radius: root.cellRadius
            font: root.font

            onReleased: root.addProject(wr.model.architecture, wr.model.abstraction, "", root.loadingFileContent, false)
            enabled: (wr.model.complete || wr.model.partiallyComplete) && (root.filterAbstraction.length === 0 || root.filterAbstraction.includes(wr.model.abstraction))
            visible: !wr.model.placeholder

            contentItem: ColumnLayout {
                anchors.centerIn: btn
                spacing: 0
                Item {  //  Spacer
                    Layout.fillHeight: true
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: wr.model.levelText
                    font.bold: true
                    font.pointSize: root.font.pointSize * 1.2
                    color: wr.model.complete ? palette.accent : settings.extPalette.brightText.background
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: "<b>" + wr.model.text + "</b> (Chap " + wr.model.chapter + ")"
                    textFormat: Text.StyledText
                    color: wr.model.complete ? palette.text : settings.extPalette.brightText.background
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: wr.model.details
                    color: wr.model.complete ? palette.text : settings.extPalette.brightText.background
                }
                Item {  //  Spacer
                    Layout.fillHeight: true
                }
            }   //  contentItem: ColumnLayout

            hoverEnabled: true
            ToolTip.visible: (btn.hovered || btn.down) && wr.model.description
            ToolTip.delay: 500
            ToolTip.text: qsTr(wr.model.description)
        }   //  RoundButton
    }   //  component ProjectButton

    ScrollView {
        id: sv

        clip: true
        anchors.fill: root

        //  Scroll bar settings
        ScrollBar.vertical.policy: layout.rowCnt > 2 ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        //  Grid containing buttons
        GridLayout {
            id: layout

            //  Grid control does not return number of rows. Rows prorperty only sets maximum rows.
            //  Must recalculate rows manually with rowChange() below.
            property int rowCnt: 1
            property int cells: root.model.rowCount()

            //  Spacing between cells
            rowSpacing: root.spacing
            columnSpacing: root.spacing

            //  There can be between 1 and 8 columns per row. If too narrow, only
            //  show visible projects and wrap
            columns: Math.min(8, Math.max(1, Math.floor(sv.width / (root.cellWidth + root.spacing))))
            Repeater {
                model: root.model
                delegate: ProjectButton {}
            }   //  Repeater
        }   //  GridLayout

        Component.onCompleted: {
            //  GridLayout does not automatically calculate number of rows
            //  based on total cells / columns.
            rowChange();
        }
        onWidthChanged: {
            //  GridLayout does not automatically calculate number of rows
            //  based on total cells / columns.
            rowChange();
        }

        function rowChange() {
            //  There is always a single row at minimum
            layout.rowCnt = Math.max(1, Math.ceil(layout.cells / layout.columns));
        }
    }   //ScrollView
}
