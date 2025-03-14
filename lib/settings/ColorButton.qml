import QtQuick
import QtQuick.Controls

//  Color picker
Item {
    id: root
    required property color color
    implicitWidth: wrapper.width
    implicitHeight: wrapper.implicitHeight

    signal requestColorChange(color color)
    TextMetrics {
        id: tm
        font: textItem.font
        // Use 9 (instead of the usual 8) 0's to ensure button widths are constant
        text: "#000000000"
    }

    Button {
        id: wrapper
        anchors.fill: parent
        highlighted: true
        width: tm.width

        contentItem: Text {
            id: textItem
            //  Color gets cast to hex value of number
            text: root.color
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            //  Use Hsv Value to pick highest contrast text
            color: root.color.hsvValue > .5 ? "black" : "white"
        }

        background: Rectangle {
            id: color
            //  Shows current color
            color: root.color
        }
    }
    Component.onCompleted: {
        wrapper.onClicked.connect(() => requestColorChange(root.color))
    }
}
