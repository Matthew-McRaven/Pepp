pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var about: Action {
        text: "&About"
        onTriggered: console.log(this.text)
    }
}
