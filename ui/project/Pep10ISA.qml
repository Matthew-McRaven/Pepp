import QtQuick 2.15
import QtQuick.Controls

import QtQuick.Layouts

Item {
    required property string mode
    readonly property variant modes: {
        "edit": edit.StackLayout.index,
        "debug": debug.StackLayout.index
    }
    StackLayout {
        id: stack
        anchors.fill: parent
        currentIndex: Qt.binding(function () {
            return modes[mode] || 0
        })
        Rectangle {
            id: edit
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: 'blue'
        }
        Rectangle {
            id: debug
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: 'green'
        }
    }
}
