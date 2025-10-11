import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property alias graphicWidth: graphicShape.width
    implicitHeight: graphicShape.height

    Shape {
        id: graphicShape
        property double lineWidth: root.graphicWidth / 6

        anchors.centerIn: parent
        width: 60
        height: 20

        ShapePath {
            strokeWidth: 1 //  Controls thickness of lines
            strokeColor: palette.text
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            startX: 0
            startY: 0

            //  Top line
            PathLine {
                x: root.graphicWidth
                y: 0
            }

            //  First diagonal
            PathMove {
                x: 10
                y: 0
            }
            PathLine {
                x: 0
                y: 10
            }
            //  Second diagonal
            PathMove {
                id: x2
                //  Non-integer values each leg to render at different thicknesses
                //  due to antialiasing. Always use integer values
                x: Math.round(graphicShape.lineWidth * 2)
                y: 0
            }
            PathLine {
                x: x2.x - 10
                y: 10
            }
            //  Third diagonal
            PathMove {
                id: x3
                x: Math.round(graphicShape.lineWidth * 3)
                y: 0
            }
            PathLine {
                x: x3.x - 10
                y: 10
            }
            //  Fourth diagonal
            PathMove {
                id: x4
                x: Math.round(graphicShape.lineWidth * 4)
                y: 0
            }
            PathLine {
                x: x4.x - 10
                y: 10
            }
            //  Fifth diagonal
            PathMove {
                id: x5
                x: Math.round(graphicShape.lineWidth * 5)
                y: 0
            }
            PathLine {
                x: x5.x - 10
                y: 10
            }
            //  Last diagonal
            PathMove {
                id: x6
                x: Math.round(root.graphicWidth)
                y: 0
            }
            PathLine {
                x: x6.x - 10
                y: 10
            }
        }
    }
}
