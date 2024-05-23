pragma Singleton

import QtQuick
import QtQuick.Controls

QtObject {
    readonly property var new_: Action {
        text: "&New"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/new.svg"
        shortcut: StandardKey.New
    }
    readonly property var open: Action {
        text: "&Open..."
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/open.svg"
        shortcut: StandardKey.Open
    }
    readonly property var save: Action {
        text: "&Save"
        onTriggered: console.log(this.text)
        icon.source: "qrc:/icons/file/save.svg"
        shortcut: StandardKey.Save
    }
    readonly property var print_: Action {
        text: "&Print"
        onTriggered: console.log(this.text)
    }
    readonly property var closeAll: Action {
        text: "Close All"
        onTriggered: console.log(this.text)
    }
    readonly property var closeAllButCurrent: Action {
        text: "Close All Except Current"
        onTriggered: console.log(this.text)
    }
    readonly property var quit: Action {
        text: "&Quit"
        onTriggered: console.log(this.text)
    }
    enum Type {
        Current = 0,
        Object = 1,
        Assembler = 2,
        Listing = 3
    }
}
