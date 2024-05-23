pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var undo: Action {
        text: "&Undo"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/undo.svg"
    }
    readonly property var redo: Action {
        text: "&Redo"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/redo.svg"
    }
    readonly property var copy: Action {
        text: "&Copy"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/copy.svg"
    }
    readonly property var cut: Action {
        text: "Cu&t"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/cut.svg"
    }
    readonly property var paste: Action {
        text: "&Paste"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/paste.svg"
    }
    readonly property var prefs: Action {
        text: "Pr&eferences"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/settings.svg"
    }
}
