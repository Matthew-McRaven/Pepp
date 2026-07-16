pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls //  SplitView
//import QtQuick.Layouts // RowLayout

import "./ui" as Ui
import "./ui/blueprint" as Bp

Window {
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")

    //Ui.LogicGates {
    //    anchors.fill: parent
    //}
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Bp.BluePrintEditor {
            SplitView.fillHeight: true
            SplitView.fillWidth: true
        }
        Bp.GateEditor {
            SplitView.fillHeight: true
            SplitView.preferredWidth: 200
            SplitView.minimumWidth: 140
        }
    }
}
