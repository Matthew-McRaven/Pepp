pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes

//import move.js as Move

Shape {
    width: 100
    height: 100
    //anchors.centerIn: parent
    ShapePath {
        strokeWidth: 1
        strokeColor: "cornflowerblue"
        strokeStyle: ShapePath.DashLine
        dashPattern: [ 1, 6 ]
        startX: 25; startY: 0
        PathLine { x: 25; y: 100 }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: "blue"
        strokeStyle: ShapePath.DashLine
        dashPattern: [ 1, 3 ]
        startX: 50; startY: 0
        PathLine { x: 50; y: 100 }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: "cornflowerblue"
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
        strokeColor: "cornflowerblue"
        strokeStyle: ShapePath.DashLine
        dashPattern: [ 1, 6 ]
        startX: 0; startY: 25
        PathLine { x: 100; y: 25 }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: "blue"
        strokeStyle: ShapePath.DashLine
        dashPattern: [ 1, 3 ]
        startX: 0; startY: 50
        PathLine { x: 100; y: 50 }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: "cornflowerblue"
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
/*Shape {
    id: root
    implicitWidth: 100//Move.majorX
    implicitHeight: 100//Move.majorY
    //anchors.fill: root

    ShapePath {
        strokeWidth: 4
        strokeColor: "blue"
        strokeStyle: ShapePath.DashLine
        dashPattern: [ 1, 4 ]

        startX: root.x;
        startY: root.y;
        PathLine { x: root.width; y: root.height }
    }
}*/
