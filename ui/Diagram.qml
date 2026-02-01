#pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.VectorImage
import QtQuick.Controls

//  Drag target
Item {
    id: root

    width: 100
    height: 100
    //required property var modelData

    //required property int column
    //required property int row
    //required property var model
    //property bool selected: false
    property string source: ""

    TableView.onPooled: image.source = ""
    TableView.onReused: image.source = root.source

    Rectangle {
        id: wrapper
        anchors.fill: parent

        width: 100
        height: 100

        color: "transparent"

        VectorImage {
            id: image

            visible: root.source != ""
            anchors.centerIn: parent
            width: wrapper.width
            height: wrapper.width / 2

            //opacity: ma.drag.active ? .25 : 1

            source: root.source === null ? "" : root.source
            fillMode: Image.PreserveAspectFit
            preferredRendererType: VectorImage.CurveRenderer
        }
    }
}
