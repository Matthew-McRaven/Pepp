pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var start: Action {
        text: "Start &Debugging"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/start_debug.svg"
    }
    readonly property var continue_: Action {
        text: "&Continue Debugging"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/continue_debug.svg"
    }
    readonly property var pause: Action {
        text: "I&nterrupt Debugging"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/pause.svg"
    }
    readonly property var stop: Action {
        text: "S&top Debugging"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/stop_debug.svg"
    }
    readonly property var step: Action {
        text: "&Step"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/step.svg"
    }
    readonly property var stepOver: Action {
        text: "Step O&ver"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/step_over.svg"
    }
    readonly property var stepInto: Action {
        text: "Step &Into"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/step_into.svg"
    }
    readonly property var stepOut: Action {
        text: "Step &Out"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/debug/step_out.svg"
    }
    readonly property var removeAllBreakpoints: Action {
        text: "&Remove All Breakpoints"
        onTriggered: console.log(this.text)
    }
}
