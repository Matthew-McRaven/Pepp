pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls //  SplitView
import QtQuick.Layouts  // StackLayout

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

        ColumnLayout {
            SplitView.fillHeight: true
            SplitView.preferredWidth: 200
            SplitView.minimumWidth: 140

            Bp.BluePrintThumbNail {
                Layout.preferredWidth: 175
                Layout.preferredHeight: 175
            }

            Rectangle {
                id: tabs

                Layout.fillHeight: true

                TabBar {
                    id: tab
                    width: tabs.width

                    TabButton {
                        text: "Gate Editor"
                    }
                    TabButton {
                        text: "Pin Editor"
                    }
                }

                StackLayout {
                    id: view
                    anchors.topMargin: tab.height + 1
                    anchors.fill: parent
                    currentIndex: tab.currentIndex

                    //  Gate selection
                    Bp.GateProperties {
                        Layout.fillWidth: true
                    }

                    //  Pin logic
                    Bp.PinProperties {
                        Layout.fillWidth: true
                    }
                }   //  StackLayout
            }   //  Rectangle
        }   //  ColumnLayout
    }   //  SplitView
}
