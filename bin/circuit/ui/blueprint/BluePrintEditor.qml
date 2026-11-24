import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "./" as Bp

SplitView {
    orientation: Qt.Horizontal
    Bp.BluePrintLibrary {
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
