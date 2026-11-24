pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

Item {
    id: root

    Pane {
        spacing: 0
        padding: 0
        anchors.fill: parent
        contentWidth: image.implicitWidth
        contentHeight: image.implicitHeight

        background: Rectangle {
            color: "#f0f0f0"
        }

        GridLines {
            anchors.fill: parent
        }

        VectorImage {
            id: image

            z: -1
            //anchors.top: parent.top
            //anchors.left: parent.left
            anchors.centerIn: parent
            width: root.width
            height: root.height * .75

            source: "qrc:/or"
            //fillMode: Image.PreserveAspectFit
            preferredRendererType: VectorImage.CurveRenderer
        }
    }   //  Pane
}
