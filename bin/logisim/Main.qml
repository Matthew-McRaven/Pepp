import QtQuick

import CircuitDesign

Window {
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")
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
    CursedCanvas{
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
    }

    Rectangle {
        id:rpanel
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: 325
        color: "red"
    }


}
