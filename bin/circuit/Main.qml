import QtQuick

import "./ui" as Ui

Window {
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")

    Ui.LogicGates {
        anchors.fill: parent
    }
}
