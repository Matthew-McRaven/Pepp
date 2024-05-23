pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var clearCPU: Action {
        text: "Clear &CPU"
        onTriggered: console.log(this.text)
    }
    readonly property var clearMemory: Action {
        text: "Clear &Memory"
        onTriggered: console.log(this.text)
    }
}
