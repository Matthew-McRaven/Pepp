pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var fullscreen: Action {
        text: "&Toggle Fullscreen"
        onTriggered: console.log(this.text)
    }
}
