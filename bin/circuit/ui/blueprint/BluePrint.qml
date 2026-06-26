pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

//  Top level window
Item {
    id: root

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        padding: 5

        //  Gate selection
        ColumnLayout {

            SplitView.preferredWidth: 205
            SplitView.maximumWidth: SplitView.preferredWidth
            SplitView.minimumWidth: SplitView.preferredWidth
            SplitView.fillHeight: true

            Image {
                id: image

                width: 205
                source: "qrc:///gatelist"
                fillMode: Image.PreserveAspectFit
            }

            Grid {
                columns: 2
                spacing: 5
                Label {
                    text: "Dimensions"
                }
                Label {
                    //  Spacer for heading
                    text: " "
                }

                Label {
                    text: "Family:"
                }
                ComboBox {
                    id: gateFamily
                    model: ["AND", "OR", "NAND", "NOR", "XOR", "Inverter"]
                    currentValue: "OR"
                }
                Label {
                    text: "Type:"
                }
                ComboBox {
                    id: gateType
                    model: ["OR 2x1"]
                    currentIndex: 0
                }

                Label {
                    text: "Orientation:"
                }
                ComboBox {
                    id: orientation
                    model: [
                        {
                            value: 0,
                            text: "Right"
                        },
                        {
                            value: 90,
                            text: "Bottom"
                        },
                        {
                            value: 180,
                            text: "Left"
                        },
                        {
                            value: 270,
                            text: "Top"
                        }
                    ]
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: 0
                }
                Label {
                    text: "Height"
                }
                SpinBox {
                    from: 2
                    to: 10
                    stepSize: 1
                    value: 2
                }
                Label {
                    text: "Width"
                }
                SpinBox {
                    from: 3
                    to: 10
                    stepSize: 1
                    value: 3
                }
                Item {
                    //  Spacer for heading
                }
            }   //  Grid

            Item {
                //  A spacer
                Layout.fillHeight: true
            }
        }   //  ColumnLayout

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
