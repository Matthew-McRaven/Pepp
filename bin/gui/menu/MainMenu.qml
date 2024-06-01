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
    TextMetrics {
        id: tm
        font: wrapper.font
        text: "  "
    }

    Menu {
        id: fileMenu
        title: qsTr("&File")
        ShortcutMenuItem {
            action: actions.file.new_
        }
        ShortcutMenuItem {
            action: actions.file.open
        }
        Menu {
            id: recentMenu
            title: "Recent Files"
            Instantiator {
                model: 5
                delegate: MenuItem {
                    text: `${modelData}.pep`
                    onTriggered: openRecent(modelData)
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
        MenuItem {
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
        }
        ShortcutMenuItem {
            action: actions.edit.copy
        }
        ShortcutMenuItem {
            action: actions.edit.paste
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
        MenuItem {
            action: actions.build.loadObject
        }
        MenuItem {
            action: actions.build.execute
        }
    }
    Menu {
        title: qsTr("&Debug")
        ShortcutMenuItem {
            action: actions.debug.start
        }
        MenuSeparator {}
        MenuItem {
            action: actions.debug.continue_
        }
        MenuItem {
            action: actions.debug.pause
        }
        MenuItem {
            action: actions.debug.stop
        }
        MenuSeparator {}
        MenuItem {
            action: actions.debug.stepInto
        }
        MenuItem {
            action: actions.debug.stepOver
        }
        MenuItem {
            action: actions.debug.stepOut
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
