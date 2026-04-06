pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage
import QtQuick.Layouts

import CircuitDesign

Rectangle {
    id: root

    property real cellWidth: 100

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        //  Diagram buttons
        ColumnLayout {

            SplitView.preferredWidth: root.cellWidth * 2 + 5
            SplitView.maximumWidth: SplitView.preferredWidth
            SplitView.minimumWidth: SplitView.preferredWidth

            DiagramListView {
                id: sourceListView
                project: canvas.project
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
            }

            Item {
                //  A spacer
                Layout.fillHeight: true
            }

            DiagramEditor {
                id: props
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: true
                diagramModel: null

                gateModel: null
            }
        }

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
                        buttonGroup.buttons[0].checked = true;
                    }
                    onClicked: btn => {
                        var result;
                        switch (btn.text) {
                            case "arrow":
                            result = FilterDiagramListModel.Arrow;
                            break;
                            case "diagram":
                            result = FilterDiagramListModel.Diagram;
                            break;
                            case "line":
                            result = FilterDiagramListModel.Line;
                            break;
                        }

                        if (result === null)
                        return;
                        canvas.filter = result;
                    }
                }

                RowLayout {
                    id: selector
                    spacing: 3

                    Button {
                        implicitHeight: 32
                        implicitWidth: 32
                        display: AbstractButton.IconOnly
                        checkable: true

                        text: "arrow"
                        icon.source: "qrc:/select"
                        icon.color: "black"
                    }
                    Button {
                        id: btn
                        implicitHeight: 32
                        implicitWidth: 32
                        display: AbstractButton.IconOnly
                        checkable: true

                        text: "diagram"
                        icon.source: "qrc:/or"
                        icon.color: "black"
                        icon.height: icon.width * .75
                    }
                    Button {
                        implicitHeight: 32
                        implicitWidth: 32
                        display: AbstractButton.IconOnly
                        checkable: true

                        text: "line"
                        icon.source: "qrc:/line"
                        icon.color: "black"
                    }
                }
            }   //  Pane

            DiagramCanvas {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true

                blueprint: sourceListView.blueprint
                filter: FilterDiagramListModel.None
                z: -1
            }
        }   //  ColumnLayout
    }   //  SplitView
}
