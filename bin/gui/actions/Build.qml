pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var loadObject: Action {
        text: "&Load Object Code"
        onTriggered: console.log(this.text)
    }
    readonly property var execute: Action {
        text: "&Execute"
        onTriggered: console.log(this.text)
    }
}
