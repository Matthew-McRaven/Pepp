import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "./" as Bp

SplitView {
    orientation: Qt.Horizontal

    Dialog {
        id: pineditDialog
        modal: false
        standardButtons: Dialog.Ok
        Bp.BluePrintPopOut {
            anchors.fill: parent
        }
        width: parent.width * 3 / 4
        height: parent.height * 3 / 4
    }

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
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("Thumbnail clicked");
                    pineditDialog.open();
                }
            }
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
