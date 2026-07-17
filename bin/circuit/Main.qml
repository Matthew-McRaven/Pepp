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
            SplitView.preferredWidth: root.preferredWidth
            SplitView.minimumWidth: 140

            Bp.BluePrintThumbNail {
                Layout.preferredWidth: root.preferredWidth
                Layout.preferredHeight: root.preferredWidth
            }

            Rectangle {
                id: tabs

                Layout.fillHeight: true
                Layout.preferredWidth: root.preferredWidth

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
