pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property alias text: label.text
    //property ali

    width: 75
    height: 50

    border.color: "blue"
    border.width: 1
    //color: "blue"
    Label {
        id: label
        text: ""

        anchors.centerIn: root
        //z:-1
    }

    MouseArea {
        anchors.fill: root
    }
}
