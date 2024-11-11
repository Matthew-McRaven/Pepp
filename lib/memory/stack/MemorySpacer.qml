import QtQuick
import QtQuick.Shapes

Rectangle {
    id: wrapper
    property double lineWidth: lineShape.width

    Shape {
        id: lineShape
        anchors.verticalCenter: parent.verticalCenter
        width: 60
        height: 10

        ShapePath {
            strokeWidth: 3 //  Controls thickness of lines
            strokeColor: palette.text
            fillColor: "transparent"
            strokeStyle: ShapePath.DashLine
            dashPattern: [5, 3]

            startX: 0
            startY: 5

            //  Dash, vertically centered
            PathLine {
                x: lineWidth
                y: 5
            }
        }
    }


    /*Component.onCompleted: {
        console.log("Memory Spacer Height / Width: " + wrapper.height + "/" + wrapper.width)
    }*/
}
