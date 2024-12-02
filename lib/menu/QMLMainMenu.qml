import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./"
// For PlatformDetector
import edu.pepp 1.0

MenuBar {
    id: wrapper
    required property var window
    required property var project
    required property Actions actions

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
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.file.save
            text: "&Save"
        }
        MenuSeparator {
            id: _closePrev
        }
        ShortcutMenuItem {
            action: actions.file.closeAll
        }
        ShortcutMenuItem {
            action: actions.file.closeAllButCurrent
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
        ShortcutMenuItem {
            action: actions.edit.prefs
        }
        ShortcutMenuItem {
            action: actions.help.about
        }
    }
    Menu {
        id: build
        title: qsTr("&Build")
        ShortcutMenuItem {
            action: actions.build.formatObject
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
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
            action: actions.build.assembleThenLoad
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
        ShortcutMenuItem {
            action: actions.debug.continue_
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.debug.pause
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.debug.stop
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.debug.stepInto
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.debug.stepOver
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        ShortcutMenuItem {
            action: actions.debug.stepOut
            enabled: action.enabled
            onEnabledChanged: contentItem.enabled = enabled
            contentItem.onEnabledChanged: fixTextColors(this)
            onPaletteChanged: fixTextColors(this)
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.debug.removeAllBreakpoints
        }
    }
    Menu {
        title: qsTr("&Simulator")
        ShortcutMenuItem {
            enabled: actions.sim.clearCPU.enabled
            action: actions.sim.clearCPU
        }
        ShortcutMenuItem {
            enabled: actions.sim.clearMemory.enabled
            action: actions.sim.clearMemory
        }
    }
    Menu {
        title: qsTr("&View")
        ShortcutMenuItem {
            enabled: !PlatformDetector.isWASM
            visible: enabled
            height: enabled ? implicitHeight : 0
            action: actions.view.fullscreen
        }
        // Dynamic magic to mode switch!
    }
    // Only meant for testing the app, not meant for deployment to users!
    Menu {
        title: qsTr("App D&ev")
        ShortcutMenuItem {
            text: qsTr("Clear Changelog Cache")
            onTriggered: actions.appdev.clearChangelogCache.trigger()
        }
    }
}
