import QtQuick

import "./ui" as Ui
import "./ui/blueprint" as Bp

Window {
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")

    Bp.BluePrint {
        anchors.fill: parent
    }
    /*Ui.LogicGates {
        anchors.fill: parent
    }*/
}
