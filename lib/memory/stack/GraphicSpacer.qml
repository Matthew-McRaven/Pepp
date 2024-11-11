import QtQuick
import QtQuick.Shapes

Item {
    Shape {
        id: graphicShape
        anchors.centerIn: parent
        width: 60
        height: 20

        ShapePath {
            strokeWidth: 3 //  Controls thickness of lines
            strokeColor: palette.text
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            startX: 0
            startY: 0
            //  Top line
            //  Top line
            PathLine {
                x: 60
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
                x: 20
                y: 0
            }
            PathLine {
                x: 10
                y: 10
            }
            //  Third diagonal
            PathMove {
                x: 30
                y: 0
            }
            PathLine {
                x: 20
                y: 10
            }
            //  Fourth diagonal
            PathMove {
                x: 40
                y: 0
            }
            PathLine {
                x: 30
                y: 10
            }
            //  Fifth diagonal
            PathMove {
                x: 50
                y: 0
            }
            PathLine {
                x: 40
                y: 10
            }
            //  Last diagonal
            PathMove {
                x: 60
                y: 0
            }
            PathLine {
                x: 50
                y: 10
            }
        }
    }
}
