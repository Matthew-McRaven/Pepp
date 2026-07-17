pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    width: 100
    height: 100

    property color lineColor: "cornflowerblue" // "gainsboro", "cornflowerblue", "blue"
    property var strokeStyle: ShapePath.DashLine // ShapePath.SolidLine
    property var dashPattern: [1, 3] // [ 1, 3 ], [1, 6]

    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 25
        startY: 0
        PathLine {
            x: 25
            y: 100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 50
        startY: 0
        PathLine {
            x: 50
            y: 100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 75
        startY: 0
        PathLine {
            x: 75
            y: 100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 100
        startY: 0
        PathLine {
            x: 100
            y: 100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: 25
        PathLine {
            x: 100
            y: 25
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: 50
        PathLine {
            x: 100
            y: 50
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: 75
        PathLine {
            x: 100
            y: 75
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: 100
        PathLine {
            x: 100
            y: 100
        }
    }
}
