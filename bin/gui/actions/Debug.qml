pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var start: Action {
        text: "&Start Debugging"
        onTriggered: console.log(this.text)
    }
    readonly property var continue_: Action {
        text: "&Continue Debugging"
        onTriggered: console.log(this.text)
    }
    readonly property var pause: Action {
        text: "I&nterrupt Debugging"
        onTriggered: console.log(this.text)
    }
    readonly property var stop: Action {
        text: "S&top Debugging"
        onTriggered: console.log(this.text)
    }
    readonly property var stepOver: Action {
        text: "&Step Over"
        onTriggered: console.log(this.text)
    }
    readonly property var stepInto: Action {
        text: "Step &Into"
        onTriggered: console.log(this.text)
    }
    readonly property var stepOut: Action {
        text: "Step &Out"
        onTriggered: console.log(this.text)
    }
    readonly property var removeAllBreakpoints: Action {
        text: "&Remove All Breakpoints"
        onTriggered: console.log(this.text)
    }
}
