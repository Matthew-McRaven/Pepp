pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes

Rectangle {
    id: root

    property bool current: false

    width: 100
    height: 100
    color: "white"
    border {
        width: root.current ? 2 : 0
        color: root.palette.highlight
    }

    Shape {
        //width: 100
        //height: 100
        anchors.fill: parent
        ShapePath {
            strokeWidth: 1
            strokeColor: "gainsboro"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 6 ]
            startX: 25; startY: 0
            PathLine { x: 25; y: 100 }
        }
        ShapePath {
            strokeWidth: 1
            strokeColor: "cornflowerblue"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 3 ]
            startX: 50; startY: 0
            PathLine { x: 50; y: 100 }
        }
        ShapePath {
            strokeWidth: 1
            strokeColor: "gainsboro"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 6 ]
            startX: 75; startY: 0
            PathLine { x: 75; y: 100 }
        }
        ShapePath {
            strokeWidth: 1
            strokeColor: "blue"
            strokeStyle: ShapePath.SolidLine
            startX: 100; startY: 0
            PathLine { x: 100; y: 100 }
        }
        ShapePath {
            strokeWidth: 1
            strokeColor: "gainsboro"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 6 ]
            startX: 0; startY: 25
            PathLine { x: 100; y: 25 }
        }
        ShapePath {
            strokeWidth: 1
            strokeColor: "cornflowerblue"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 3 ]
            startX: 0; startY: 50
            PathLine { x: 100; y: 50 }
        }
        ShapePath {
            strokeWidth: 1
            strokeColor: "gainsboro"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 6 ]
            startX: 0; startY: 75
            PathLine { x: 100; y: 75 }
        }
        ShapePath {
            strokeWidth: 1
            strokeColor: "blue"
            strokeStyle: ShapePath.SolidLine
            startX: 0; startY: 100
            PathLine { x: 100; y: 100 }
        }
    }
}
