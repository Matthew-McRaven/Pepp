pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

Item {
    id: root
    /*ScrollView {
        id: scrollView
        anchors.fill: parent

        contentItem:*/
    Pane {
        spacing: 0
        anchors.fill: parent
        contentWidth: image.implicitWidth
        contentHeight: image.implicitHeight

        background: Rectangle {
            color: "#f0f0f0"
        }

        Column {
            spacing: 0
            anchors.top: parent.top
            anchors.left: parent.left

            Repeater {
                id: columns
                model: 10
                Row {
                    spacing: 0
                    Repeater {
                        id: rows
                        model: 10
                        GridLines {}
                    }
                }
            }
        }   //  contentItem

        VectorImage {
            id: image

            z: -1
            anchors.top: parent.top
            anchors.left: parent.left
            width: 300
            height: 200

            source: "qrc:/or"
            //fillMode: Image.PreserveAspectFit
            preferredRendererType: VectorImage.CurveRenderer
        }
    }   //  Pane
    //}   //  ScrollView
}   //  Item
