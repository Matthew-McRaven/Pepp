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
                //currentIndex: canvas.currentItem
                gateModel: sourceListView.filterList
            }
        }

        DiagramCanvas {
            id: canvas
            dataModel: diagramModel
            currentStamp: sourceListView.currentStamp
        }
    }
}
