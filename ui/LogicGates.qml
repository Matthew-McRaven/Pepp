pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage
import QtQuick.Layouts

import CircuitDesign

Rectangle {
    id: root

    property real cellWidth: 100

    DiagramDataModel {
        id: diagramModel
    }

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

                diagramModel: diagramModel
                gateModel: sourceListView.filterList
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

                RowLayout {
                    spacing: 3
                    Button {
                        implicitHeight: 32
                        implicitWidth: 32
                        display: AbstractButton.IconOnly

                        icon.source: "qrc:/select"
                        icon.color: "black"
                    }
                    Button {
                        id: btn
                        implicitHeight: 32
                        implicitWidth: 32
                        display: AbstractButton.IconOnly

                        icon.source: "qrc:/or"
                        icon.color: "black"
                        icon.height: icon.width * .75
                    }
                    Button {
                        implicitHeight: 32
                        implicitWidth: 32
                        display: AbstractButton.IconOnly

                        icon.source: "qrc:/line"
                        icon.color: "black"
                    }
                }
            }   //  Pane

            DiagramCanvas {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true

                dataModel: diagramModel
                currentStamp: sourceListView.currentStamp
                z: -1
            }
        }   //  ColumnLayout
    }   //  SplitView
}
