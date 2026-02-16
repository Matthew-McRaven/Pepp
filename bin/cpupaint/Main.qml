import QtQuick

import CPUPaint
import QtQuick.Controls
import "./" as C

Window {
    id: root
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")
    // A dummy left-panel
    Rectangle {
        id: lpanel
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: 150
        color: "orange"
    }
    C.DatapathPainter {
        clip:true
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
    }
    // A dummy right-panel
    Rectangle {
        id: rpanel
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: 325
        color: "red"
    }
}
