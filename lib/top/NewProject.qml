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

    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width

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
                    color: wr.model.complete ? palette.accent : settings.extPalette.brightText.background // "#aaa"
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

    //  Layout of projects that may be picked by user.
    ListView {
        id: layout

        implicitHeight: root.cellHeight

        width: root.width
        orientation: ListView.Horizontal
        spacing: root.spacing

        model: root.model
        delegate: ProjectButton{}
    }
}
