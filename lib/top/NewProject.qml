pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    //  Grid layout
    property int columns: 8
    property real rowSpacing: 0
    property real columnSpacing: 0

    //  Grid cell layout
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
    implicitWidth: root.columns * (root.cellWidth + root.rowSpacing)

    component ProjectButton: RoundButton {
        id: btn
        implicitHeight: root.cellHeight
        implicitWidth: root.cellWidth

        //  Declare required properties
        required property int index
        required property var model

        radius: root.cellRadius
        font: root.font

        //border.width: 1
        //border.color: (mouse.containsMouse && !btn.future) ? palette.accent : palette.mid
        //color: palette.window

        onReleased: root.addProject(model.architecture, model.abstraction, "", root.loadingFileContent, false)
        enabled: (model.complete || model.partiallyComplete) && (root.filterAbstraction.length === 0 || root.filterAbstraction.includes(model.abstraction))

        contentItem: ColumnLayout {
            anchors.centerIn: btn
            spacing: 0
            Item {  //  Spacer
                Layout.fillHeight: true
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: btn.model.levelText
                font.bold: true
                font.pointSize: root.font.fontSize * 2
                color: btn.model.placeholder ? palette.mid : palette.accent
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: "<b>" + btn.model.text + "</b> (Chap " + btn.model.chapter + ")"
                textFormat: Text.StyledText
                color: btn.model.placeholder ? palette.mid : palette.text
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: btn.model.details
                color: btn.model.placeholder ? palette.mid : palette.text
            }
            Item {  //  Spacer
                Layout.fillHeight: true
            }
        }   //  contentItem: ColumnLayout

        hoverEnabled: true
        ToolTip.visible: (hovered || down) && model.description
        ToolTip.delay: 500
        ToolTip.text: qsTr(model.description)
    }   //  component ProjectBox

    //  Layout of projects that may be picked by user.
    GridLayout {
        id: grid

        width: root.width
        rowSpacing: root.rowSpacing
        columnSpacing: root.columnSpacing
        //columns: root.columns
        layoutDirection: GridLayout.LeftToRight

        Repeater {
            model: root.model
            delegate: ProjectButton {
            }
        }
        Item {
            id: spacer
            Layout.fillHeight: true
            Layout.fillWidth: true
            //Layout.columnSpan: Math.max(0,grid.columns)
        }
    }
}
