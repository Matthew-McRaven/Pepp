pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.VectorImage

//  Drag target
Item {
    id: root

    width: 100
    height: 100

    required property int row
    required property int column
    required property bool current
    required property bool editing
    required property bool selected
    required property var model

    property string source: ""

    TableView.onPooled: image.source = ""
    TableView.onReused: image.source = root.source
    GridLines {
        anchors.fill: parent
        z: -1
        current: root.current
    }

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

            source: root.source === null ? "" : root.source
            fillMode: Image.PreserveAspectFit
            preferredRendererType: VectorImage.CurveRenderer
        }
    }
}
