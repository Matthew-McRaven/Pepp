pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var loadObject: Action {
        text: "&Load Object Code"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/build/flash.svg"
    }
    readonly property var execute: Action {
        text: "&Execute"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/start_normal.svg"
    }
}
