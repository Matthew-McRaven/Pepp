pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//  Top level window
Item {
    id: root

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        padding: 5

        Rectangle {
            id: tabs

            SplitView.preferredWidth: 205
            SplitView.maximumWidth: SplitView.preferredWidth
            SplitView.minimumWidth: SplitView.preferredWidth
            SplitView.fillHeight: true

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
                GateProperties {
                    id: gates

                    Layout.fillWidth: true
                }

                Rectangle {
                    id: pins
                    color: "red"
                    Layout.fillWidth: true
                }
            }   //  StackLayout
        }

        //  Canvas area - right side
        ColumnLayout {
            spacing: 0
            Pane {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                leftPadding: 0
                rightPadding: 0
                topPadding: 3
                bottomPadding: 3
                spacing: 0

                ButtonGroup {
                    id: buttonGroup
                    buttons: selector.children

                    Component.onCompleted: {
                        //  Enable select and disable diagram selections
                        buttonGroup.buttons[0].checked = true;
                        //sourceListView.enabled = false;
                    }
                    /*onClicked: btn => {
                        var result;
                        switch (btn.text) {
                        case "arrow":
                            result = BlueprintLibraryModel.Arrow;
                            //sourceListView.enabled = false;
                            break;
                        case "diagram":
                            result = BlueprintLibraryModel.Diagram;
                            //  Blueprint is only active when in diagram mode.
                            //sourceListView.enabled = true;
                            break;
                        case "line":
                            result = BlueprintLibraryModel.Line;
                            //sourceListView.enabled = false;
                            break;
                        }

                        if (result === null)
                            return;
                        canvas.filter = result;
                    }*/
                }

                RowLayout {
                    id: selector
                    spacing: 3

                    Button {
                        implicitHeight: 40
                        implicitWidth: 60
                        //display: AbstractButton.TextUnderIcon
                        checkable: true

                        text: "Input"
                    }
                    Button {
                        id: btn
                        implicitHeight: 40
                        implicitWidth: 60
                        checkable: true

                        text: "Output"
                    }
                    Button {
                        implicitHeight: 40
                        implicitWidth: 60
                        checkable: true

                        text: "Input/Output"
                    }
                    Button {
                        implicitHeight: 40
                        implicitWidth: 60
                        checkable: true

                        text: "Clock"
                    }
                }
            }   //  Pane

            BluePrintCanvas {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
