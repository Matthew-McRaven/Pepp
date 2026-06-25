pragma ComponentBehavior: Bound
import QtQuick

import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage
import QtQuick.Layouts

Rectangle {
    id: root

    /*BlueprintLibraryModel {
        id: blueprintModel
        project: canvas.project
    }*/

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        //  Diagram buttons
        ColumnLayout {

            SplitView.preferredWidth: 205
            SplitView.maximumWidth: SplitView.preferredWidth
            SplitView.minimumWidth: SplitView.preferredWidth

            /*BluePrintListView {
                id: sourceListView
                blueprintModel: blueprintModel
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
            }

            Item {
                //  A spacer
                Layout.fillHeight: true
            }

            ComponentEditor {
                id: props
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: true
                component: canvas.componentWrapper
                blueprintModel: blueprintModel
            }*/
        }

        ColumnLayout {
            spacing: 0

            BluePrintCanvas {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }   //  ColumnLayout
    }   //  SplitView
}
