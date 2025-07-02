import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Labs
//  Native menu for apple, linux, and windows
import "./"

Labs.MenuBar {
    id: wrapper
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
        for (var i = 0; i < menu.data.length; i++) {
            //  See if item is same as list
            if (menu.data[i] === menuItem) {
                return i;
            }
        }
        return menu.data.length;
    }
    function updateDarkMode() {
        wrapper.darkMode = Qt.binding(() => Application.styleHints.colorScheme === Qt.ColorScheme.Dark);
    }
    NuAppSettings {
        id: settings
    }

    Component.onCompleted: {
        Application.styleHints.colorSchemeChanged.connect(() => wrapper.updateDarkMode);
    }
    Labs.Menu {
        id: fileMenu
        title: qsTr("&File")
        Labs.MenuItem {
            id: new_
            text: actions.file.new_.text
            onTriggered: actions.file.new_.trigger()
            icon.source: fixSuffix(actions.file.new_.icon.source, wrapper.darkMode)
            shortcut: actions.file.new_.shortcut
        }
        Labs.MenuItem {
            text: actions.file.open.text
            onTriggered: actions.file.open.trigger()
            icon.source: fixSuffix(actions.file.open.icon.source, wrapper.darkMode)
            shortcut: actions.file.open.shortcut
        }

        Labs.Menu {
            id: recentFilesMenu
            title: qsTr("&Recent Files...")
            Instantiator {
                id: recentFilesInstantiator
                model: settings.general.recentFiles
                delegate: Labs.MenuItem {
                    required property var model
                    text: settings.general.fileNameFor(model.modelData)
                    onTriggered: {
                        actions.window.onOpenFile(model.modelData);
                    }
                }
                onObjectAdded: (index, object) => recentFilesMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => recentFilesMenu.removeItem(object)
            }
            Labs.MenuSeparator {}
            Labs.MenuItem {
                text: actions.file.clearRecents.text
                onTriggered: actions.file.clearRecents.trigger()
                icon.source: fixSuffix(actions.file.clearRecents.icon.source, wrapper.darkMode)
                shortcut: actions.file.clearRecents.shortcut
            }
        }

        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: actions.file.save.text
            onTriggered: actions.file.save.trigger()
            icon.source: fixSuffix(actions.file.save.icon.source, wrapper.darkMode)
            shortcut: actions.file.save.shortcut
        }
        Component {
            id: saveAsComponent
            Labs.MenuItem {
                required property string extension
                text: `Save ${extension} as...`
                onTriggered: {
                    actions.window.onSaveAs(extension);
                }
            }
        }

        Labs.Menu {
            id: saveAsMenu
            title: qsTr("Save as...")
            visible: saveAsInstantiator.model.length > 0
            Instantiator {
                id: saveAsInstantiator
                model: project?.saveAsOptions ?? []
                onModelChanged: update()
                function update() {
                    saveAsMenu.clear();
                    const newCount = model.length;
                    for (let it = 0; it < newCount; it++) {
                        const props = {
                            extension: model[it]
                        };
                        saveAsMenu.addItem(saveAsComponent.createObject(saveAsMenu, props));
                    }
                }

                Component.onCompleted: {
                    wrapper.onProjectChanged.connect(update);
                }
            }
        }
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: actions.edit.prefs.text
            onTriggered: actions.edit.prefs.trigger()
            icon.source: fixSuffix(actions.edit.prefs.icon.source, wrapper.darkMode)
        }
        Labs.MenuItem {
            text: actions.file.quit.text
            onTriggered: actions.file.quit.trigger()
            icon.source: fixSuffix(actions.file.quit.icon.source, wrapper.darkMode)
            shortcut: actions.file.quit.shortcut
        }
    }
    Labs.Menu {
        title: qsTr("&Edit")
        Labs.MenuItem {
            text: actions.edit.undo.text
            onTriggered: actions.edit.undo.trigger()
            icon.source: fixSuffix(actions.edit.undo.icon.source, wrapper.darkMode)
            shortcut: actions.edit.undo.shortcut
        }
        Labs.MenuItem {
            text: actions.edit.redo.text
            onTriggered: actions.edit.redo.trigger()
            icon.source: fixSuffix(actions.edit.redo.icon.source, wrapper.darkMode)
            shortcut: actions.edit.redo.shortcut
        }
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: actions.edit.cut.text
            onTriggered: actions.edit.cut.trigger()
            icon.source: fixSuffix(actions.edit.cut.icon.source, wrapper.darkMode)
            enabled: actions.edit.cut.enabled
            shortcut: actions.edit.cut.shortcut
        }
        Labs.MenuItem {
            text: actions.edit.copy.text
            onTriggered: actions.edit.copy.trigger()
            icon.source: fixSuffix(actions.edit.copy.icon.source, wrapper.darkMode)
            enabled: actions.edit.copy.enabled
            shortcut: actions.edit.copy.shortcut
        }
        Labs.MenuItem {
            text: actions.edit.paste.text
            onTriggered: actions.edit.paste.trigger()
            icon.source: fixSuffix(actions.edit.paste.icon.source, wrapper.darkMode)
            enabled: actions.edit.paste.enabled
            shortcut: actions.edit.paste.shortcut
        }
        // Formatting magic!
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: actions.build.formatObject.text
            onTriggered: actions.build.formatObject.trigger()
            enabled: actions.build.formatObject.enabled
            visible: enabled
            icon.source: fixSuffix(actions.build.formatObject.icon.source, wrapper.darkMode)
        }
        Labs.MenuItem {
            text: actions.build.assembleThenFormat.text
            onTriggered: actions.build.assembleThenFormat.trigger()
            enabled: actions.build.assembleThenFormat.enabled
            visible: enabled
            icon.source: fixSuffix(actions.build.assembleThenFormat.icon.source, wrapper.darkMode)
        }
    }
    Labs.Menu {
        id: build
        title: qsTr("&Build")

        Labs.MenuItem {
            text: actions.build.loadObject.text
            onTriggered: actions.build.loadObject.trigger()
            enabled: actions.build.loadObject.enabled
            visible: enabled
            icon.source: fixSuffix(actions.build.loadObject.icon.source, wrapper.darkMode)
            shortcut: actions.build.loadObject.shortcut
        }
        Labs.MenuItem {
            text: actions.build.assemble.text
            onTriggered: actions.build.assemble.trigger()
            enabled: actions.build.assemble.enabled
            visible: enabled
            icon.source: fixSuffix(actions.build.assemble.icon.source, wrapper.darkMode)
            shortcut: actions.build.assemble.shortcut
        }
        Labs.MenuItem {
            text: actions.build.assembleThenLoad.text
            onTriggered: actions.build.assembleThenLoad.trigger()
            enabled: actions.build.assembleThenLoad.enabled
            visible: enabled
            icon.source: fixSuffix(actions.build.assembleThenLoad.icon.source, wrapper.darkMode)
            shortcut: actions.build.assembleThenLoad.shortcut
        }
        Labs.MenuItem {
            text: actions.build.execute.text
            onTriggered: actions.build.execute.trigger()
            enabled: actions.build.execute.enabled
            icon.source: fixSuffix(actions.build.execute.icon.source, wrapper.darkMode)
            shortcut: actions.build.execute.shortcut
        }
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: actions.help.about.text
            onTriggered: actions.help.about.trigger()
        }
    }
    Labs.Menu {
        title: qsTr("&Debug")
        Labs.MenuItem {
            text: actions.debug.start.text
            onTriggered: actions.debug.start.trigger()
            enabled: actions.debug.start.enabled
            icon.source: fixSuffix(actions.debug.start.icon.source, wrapper.darkMode)
            shortcut: actions.debug.start.shortcut
        }
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: actions.debug.continue_.text
            onTriggered: actions.debug.continue_.trigger()
            enabled: actions.debug.continue_.enabled
            icon.source: fixSuffix(actions.debug.continue_.icon.source, wrapper.darkMode)
        }
        Labs.MenuItem {
            text: actions.debug.pause.text
            onTriggered: actions.debug.pause.trigger()
            enabled: actions.debug.pause.enabled
            icon.source: fixSuffix(actions.debug.pause.icon.source, wrapper.darkMode)
            shortcut: actions.debug.pause.shortcut
        }
        Labs.MenuItem {
            text: actions.debug.stop.text
            onTriggered: actions.debug.stop.trigger()
            enabled: actions.debug.stop.enabled
            icon.source: fixSuffix(actions.debug.stop.icon.source, wrapper.darkMode)
        }
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: actions.debug.stepInto.text
            onTriggered: actions.debug.stepInto.trigger()
            enabled: actions.debug.stepInto.enabled
            icon.source: fixSuffix(actions.debug.stepInto.icon.source, wrapper.darkMode)
        }
        Labs.MenuItem {
            text: actions.debug.stepOver.text
            onTriggered: actions.debug.stepOver.trigger()
            enabled: actions.debug.stepOver.enabled
            icon.source: fixSuffix(actions.debug.stepOver.icon.source, wrapper.darkMode)
        }
        Labs.MenuItem {
            text: actions.debug.stepOut.text
            onTriggered: actions.debug.stepOut.trigger()
            enabled: actions.debug.stepOut.enabled
            icon.source: fixSuffix(actions.debug.stepOut.icon.source, wrapper.darkMode)
        }
        Labs.MenuSeparator {}
        Labs.MenuItem {
            text: qsTr("&Remove All Breakpoints")
            onTriggered: actions.debug.removeAllBreakpoints.trigger()
        }
    }
    Labs.Menu {
        title: qsTr("&Simulator")
        Labs.MenuItem {
            enabled: actions.sim.clearCPU.enabled
            text: actions.sim.clearCPU.text
            onTriggered: actions.sim.clearCPU.trigger()
        }
        Labs.MenuItem {
            enabled: actions.sim.clearMemory.enabled
            text: actions.sim.clearMemory.text
            onTriggered: actions.sim.clearMemory.trigger()
        }
    }
    Labs.Menu {
        title: qsTr("&View")
        Labs.MenuItem {
            text: actions.view.fullscreen.text
            onTriggered: actions.view.fullscreen.trigger()
        }
    }
    // Only meant for testing the app, not meant for deployment to users!
    Labs.Menu {
        visible: settings.general.showDebugComponents
        enabled: visible
        title: qsTr("App D&ev")
        Labs.MenuItem {
            text: qsTr("Clear Changelog Cache")
            onTriggered: actions.appdev.clearChangelogCache.trigger()
        }
        // Not present in QMLMainMenu, since there is no way to get many files into that disk in WASM.
        Labs.MenuItem {
            text: qsTr("&Reload Figures")
            onTriggered: actions.appdev.reloadFigures.trigger()
        }
    }
}
