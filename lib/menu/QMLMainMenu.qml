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
    property bool darkMode: Application.styleHints.colorScheme === Qt.ColorScheme.Dark
    // Must pass wrapper.darkMode as the 2nd parameter so that updates to application color scheme will
    // cascade through icon.source in menu bar.
    function fixSuffix(source, useDark) {
        // Coerce to JS string so that replace works correctly.
        const withoutColor = ("" + source).replace(/_dark/i, "");
        const ret = withoutColor.replace(/\.svg/i, `${useDark ? "_dark" : ""}.svg`);
        return ret;
    }
    function indexOf(menu, menuItem) {
        for (var i = 0; i < menu.count; i++) {
            if (menu.itemAt(i) === menuItem) {
                return i;
            }
        }
        return menu.count;
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
            icon.source: fixSuffix(actions.file.new_.icon.source, wrapper.darkMode)
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.file.save
            text: "&Save"
            icon.source: fixSuffix(actions.file.save.icon.source, wrapper.darkMode)
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
        ShortcutMenuItem {
            action: actions.edit.prefs
            icon.source: fixSuffix(actions.edit.prefs.icon.source, wrapper.darkMode)
        }
    }
    Menu {
        title: qsTr("&Edit")
        ShortcutMenuItem {
            action: actions.edit.undo
            icon.source: fixSuffix(actions.edit.undo.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.edit.redo
            icon.source: fixSuffix(actions.edit.redo.icon.source, wrapper.darkMode)
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.edit.cut
            enabled: action.enabled
            icon.source: fixSuffix(actions.edit.cut.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.edit.copy
            enabled: action.enabled
            icon.source: fixSuffix(actions.edit.copy.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.edit.paste
            enabled: action.enabled
            icon.source: fixSuffix(actions.edit.paste.icon.source, wrapper.darkMode)
        }
        // Formatting magic!
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.build.formatObject
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            onEnabledChanged: contentItem.enabled = enabled
        }
        ShortcutMenuItem {
            action: actions.build.assembleThenFormat
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            icon.source: fixSuffix(actions.build.assembleThenFormat.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.edit.clearEditorErrors
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            icon.source: fixSuffix(actions.edit.clearEditorErrors.icon.source, wrapper.darkMode)
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
            icon.source: fixSuffix(actions.build.loadObject.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.build.assemble
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            icon.source: fixSuffix(actions.build.assemble.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.build.assembleThenLoad
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            icon.source: fixSuffix(actions.build.assembleThenLoad.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.microAssemble.assemble
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            icon.source: fixSuffix(actions.build.microAssemble.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.build.microAssembleThenFormat
            enabled: action.enabled
            visible: enabled
            height: enabled ? implicitHeight : 0
            icon.source: fixSuffix(actions.build.microAssembleThenFormat.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.build.execute
            enabled: action.enabled
            icon.source: fixSuffix(actions.build.execute.icon.source, wrapper.darkMode)
        }
    }
    Menu {
        title: qsTr("&Debug")
        ShortcutMenuItem {
            action: actions.debug.start
            enabled: action.enabled
            icon.source: fixSuffix(actions.debug.start.icon.source, wrapper.darkMode)
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.debug.continue_
            enabled: action.enabled
            icon.source: fixSuffix(actions.debug.continue_.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.debug.pause
            enabled: action.enabled
            icon.source: fixSuffix(actions.debug.pause.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.debug.stop
            enabled: action.enabled
            icon.source: fixSuffix(actions.debug.stop.icon.source, wrapper.darkMode)
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.debug.stepInto
            enabled: action.enabled
            icon.source: fixSuffix(actions.debug.stepInto.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.debug.stepOver
            enabled: action.enabled
            icon.source: fixSuffix(actions.debug.stepOver.icon.source, wrapper.darkMode)
        }
        ShortcutMenuItem {
            action: actions.debug.stepOut
            enabled: action.enabled
            icon.source: fixSuffix(actions.debug.stepOut.icon.source, wrapper.darkMode)
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
        title: qsTr("&Help")
        ShortcutMenuItem {
            action: actions.help.about
        }
        MenuSeparator {}
        ShortcutMenuItem {
            action: actions.help.resetSettings
        }
    }

    // Only meant for testing the app, not meant for deployment to users!
    Menu {
        title: qsTr("App D&ev")
        ShortcutMenuItem {
            text: qsTr("Clear Changelog Cache")
            onTriggered: actions.appdev.clearChangelogCache.trigger()
        }
        ShortcutMenuItem {
            text: actions.appdev.openSelftest.text
            onTriggered: actions.appdev.openSelftest.trigger()
        }
    }
}
