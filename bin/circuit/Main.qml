pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls //  SplitView
import QtQuick.Layouts  // StackLayout

import "./ui" as Ui
import "./ui/blueprint" as Bp

Window {
    id: root
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")

    property int preferredWidth: 200
    //
    TabBar {
        id: bar
        width: parent.width
        TabButton {
            text: qsTr("Library")
        }
        TabButton {
            text: qsTr("Logic Gates")
        }
    }
    StackLayout {
        currentIndex: bar.currentIndex
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        Bp.BluePrintEditor {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        Ui.LogicGates {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    } // StackLayout
}
