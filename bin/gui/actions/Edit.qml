pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var undo: Action {
        text: "&Undo"
        onTriggered: console.log(this.text)
    }
    readonly property var redo: Action {
        text: "&Redo"
        onTriggered: console.log(this.text)
    }
    readonly property var copy: Action {
        text: "&Copy"
        onTriggered: console.log(this.text)
    }
    readonly property var cut: Action {
        text: "Cu&t"
        onTriggered: console.log(this.text)
    }
    readonly property var paste: Action {
        text: "&Paste"
        onTriggered: console.log(this.text)
    }
    readonly property var prefs: Action {
        text: "Pr&eferences"
        onTriggered: console.log(this.text)
    }
}
