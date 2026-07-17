pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    implicitWidth: 100
    implicitHeight: 100

    property color lineColor: "cornflowerblue" // "gainsboro", "cornflowerblue", "blue"
    property var strokeStyle: ShapePath.DashLine // ShapePath.SolidLine
    property var dashPattern: [1, 3] // [ 1, 3 ], [1, 6]

    //  Vertical lines
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: 0
        PathLine {
            x: 0
            y: root.height          //  100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: root.width / 4      //  25
        startY: 0
        PathLine {
            x: root.width / 4       //  25
            y: root.height          //  100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: root.width / 2      //  50
        startY: 0
        PathLine {
            x: root.width / 2       //  50
            y: root.height          //  100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: root.width * .75    //  75
        startY: 0
        PathLine {
            x: root.width * .75     //  75
            y: root.height          //  100
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: root.width   //  100
        startY: 0
        PathLine {
            x: root.width   //  100
            y: root.height      //  100
        }
    }
    //  Horizontal lines
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: 0
        PathLine {
            x: root.width      //  100
            y: 0
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: root.height / 4 //  25
        PathLine {
            x: root.width      //  100
            y: root.height / 4 //  25
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: root.height / 2 //  50
        PathLine {
            x: root.width       //  100
            y: root.height / 2  //  50
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: root.height * .75   //  75
        PathLine {
            x: root.width           //  100
            y: root.height * .75    //  75
        }
    }
    ShapePath {
        strokeWidth: 1
        strokeColor: root.lineColor
        strokeStyle: root.strokeStyle
        dashPattern: root.dashPattern
        startX: 0
        startY: root.height //  100
        PathLine {
            x: root.width   //  100
            y: root.height  //  100
        }
    }
}
