import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"

MenuBar {
    id: wrapper
    required property var window
    required property var project
    required property Actions actions
    property alias saveAsModel: saveAsInstantiator.model
    property alias printModel: printInstantiator.model
    property alias closeModel: closeInstantiator.model

    function indexOf(menu, menuItem) {
        for (var i = 0; i < menu.count; i++) {
            if (menu.itemAt(i) === menuItem) {
                return i
            }
        }
        return menu.count
    }
    function fixTextColors(item) {
        const p = item.palette
        const en = p.text, dis = p.disabled.text
        // TODO: Remove when themes work properly
        // TODO: fix icon colors
        const ens = Qt.rgba(en.r + 0.1, en.g + 0.1, en.b + 0.1, en.a)
        // Color to pick if enabled == disabled, and that color is #000000, on which lighter does not work.
        const backup = en.hslLightness < 0.5 ? ens.lighter(4.0) : en.darker()
        const selected = item.enabled ? en : (en === dis ? backup : dis)
        if (item.color)
            item.color = Qt.binding(() => selected)
        item.contentItem.color = selected
    }
    TextMetrics {
        id: tm
        font: wrapper.font
        text: "  "
    }

    Menu {
        id: fileMenu
        title: qsTr("&File")
        ShortcutMenuItem {
            id: new_
            action: actions.file.new_
        }
        ShortcutMenuItem {
            action: actions.file.open
        }
        Menu {
            id: recentMenu
            title: "Recent Files"
            // Use blank icon to force menu items to line up. Do not use image provider for a Menu item, since
            // this icon is rendered before the image provider's paint engine is set up.
            icon.source: "qrc:/icons/blank.svg"
            // As such, the width of the icon may be wrong, so use the width of a different (working) icon.
            icon.width: new_.icon.width

            Instantiator {
                model: 5
                delegate: MenuItem {
                    text: `${modelData}.pep`
                    onTriggered: openRecent(modelData)
                    icon.source: "image://icons/blank.svg"
                }
                onObjectAdded: (i, obj) => recentMenu.insertItem(i, obj)
                onObjectRemoved: (i, obj) => recentMenu.removeItem(obj)
            }
        }

        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.file.save
            text: "&Save Object"
        }
        MenuSeparator {
            id: _saveAsPrev
        }
        Instantiator {
            id: saveAsInstantiator
            model: 2
            delegate: MenuItem {
                text: "Save as" + modelData
                onTriggered: saveAs(modelData)
                // Use blank icon to force menu items to line up.
                icon.source: "image://icons/blank.svg"
            }
            onObjectAdded: function (index, object) {
                const m = fileMenu
                m.insertItem(index + indexOf(m, _saveAsPrev) + 1, object)
            }
            onObjectRemoved: (index, object) => fileMenu.removeItem(object)
        }
        MenuSeparator {
            id: _printPrev
        }
        Instantiator {
            id: printInstantiator
            model: 3
            delegate: MenuItem {
                text: "Print" + modelData
                onTriggered: print_(modelData)
                // Use blank icon to force menu items to line up.
                icon.source: "image://icons/blank.svg"
            }
            onObjectAdded: function (index, object) {
                const m = fileMenu
                m.insertItem(index + indexOf(m, _printPrev) + 1, object)
            }
            onObjectRemoved: (index, object) => fileMenu.removeItem(object)
        }
        MenuSeparator {
            id: _closePrev
        }
        Instantiator {
            id: closeInstantiator
            model: 3
            delegate: MenuItem {
                text: "Close" + modelData
                onTriggered: close(modelData)
                // Use blank icon to force menu items to line up.
                icon.source: "image://icons/blank.svg"
            }
            onObjectAdded: function (index, object) {
                const m = fileMenu
                m.insertItem(index + indexOf(m, _closePrev) + 1, object)
            }
            onObjectRemoved: (index, object) => fileMenu.removeItem(object)
        }
        MenuItem {
            action: actions.file.closeAll
        }
        MenuItem {
            action: actions.file.closeAllButCurrent
        }

        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.file.quit
        }
    }
    Menu {
        title: qsTr("&Edit")
        ShortcutMenuItem {
            action: actions.edit.undo
        }
        ShortcutMenuItem {
            action: actions.edit.redo
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.edit.cut
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.edit.copy
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.edit.paste
            onPaletteChanged: fixTextColors(this)
        }
        // Formatting magic!
        MenuSeparator {}
        MenuItem {
            action: actions.edit.prefs
        }
    }
    Menu {
        id: build
        title: qsTr("&Build")
        ShortcutMenuItem {
            action: actions.build.loadObject
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.build.assemble
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.build.assembleThenFormat
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.build.execute
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
    }
    Menu {
        title: qsTr("&Debug")
        ShortcutMenuItem {
            action: actions.debug.start
            enabled: action.enabled
            contentItem.enabled: action.enabled
            onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuSeparator {}
        MenuItem {
            action: actions.debug.continue_
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuItem {
            action: actions.debug.pause
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuItem {
            action: actions.debug.stop
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuSeparator {}
        MenuItem {
            action: actions.debug.stepInto
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuItem {
            action: actions.debug.stepOver
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuItem {
            action: actions.debug.stepOut
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuSeparator {}
        MenuItem {
            action: actions.debug.removeAllBreakpoints
        }
    }
    Menu {
        title: qsTr("&Simulator")
        MenuItem {
            action: actions.sim.clearCPU
        }
        MenuItem {
            action: actions.sim.clearMemory
        }
    }
    Menu {
        title: qsTr("&View")
        MenuItem {
            action: actions.view.fullscreen
        }
        // Dynamic magic to mode switch!
    }
    Menu {
        title: qsTr("&Help")
        MenuItem {
            action: actions.help.about
        }
    }
}
