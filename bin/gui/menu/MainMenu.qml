import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp.Actions as Actions

MenuBar {
    property alias saveAsModel: saveAsInstantiator.model
    property alias printModel: printInstantiator.model
    property alias closeModel: closeInstantiator.model
    signal saveAs(int type)
    signal print(int type)
    signal openRecent(string path)
    signal close(int index)

    function indexOf(menu, menuItem) {
        for (var i = 0; i < menu.count; i++) {
            if (menu.itemAt(i) === menuItem) {
                return i
            }
        }
        return menu.count
    }

    Menu {
        id: fileMenu
        title: qsTr("&File")
        MenuItem {
            action: Actions.File.new_
        }
        MenuItem {
            action: Actions.File.open
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
        MenuItem {
            action: Actions.File.save
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
                onTriggered: print(modelData)
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
            action: Actions.File.closeAll
        }
        MenuItem {
            action: Actions.File.closeAllButCurrent
        }

        MenuSeparator {}
        MenuItem {
            action: Actions.File.quit
        }
    }
    Menu {
        title: qsTr("&Edit")
        MenuItem {
            action: Actions.Edit.undo
        }
        MenuItem {
            action: Actions.Edit.redo
        }
        MenuSeparator {}
        MenuItem {
            action: Actions.Edit.cut
        }
        MenuItem {
            action: Actions.Edit.copy
        }
        MenuItem {
            action: Actions.Edit.paste
        }
        // Formatting magic!
        MenuSeparator {}
        MenuItem {
            action: Actions.Edit.prefs
        }
    }
    Menu {
        title: qsTr("&Build")
        MenuItem {
            action: Actions.Build.loadObject
        }
        MenuItem {
            action: Actions.Build.execute
        }
    }
    Menu {
        title: qsTr("&Debug")
        MenuItem {
            action: Actions.Debug.start
        }
        MenuSeparator {}
        MenuItem {
            action: Actions.Debug.continue_
        }
        MenuItem {
            action: Actions.Debug.pause
        }
        MenuItem {
            action: Actions.Debug.stop
        }
        MenuSeparator {}
        MenuItem {
            action: Actions.Debug.stepInto
        }
        MenuItem {
            action: Actions.Debug.stepOver
        }
        MenuItem {
            action: Actions.Debug.stepOut
        }
        MenuSeparator {}
        MenuItem {
            action: Actions.Debug.removeAllBreakpoints
        }
    }
    Menu {
        title: qsTr("&Simulator")
        MenuItem {
            action: Actions.Sim.clearCPU
        }
        MenuItem {
            action: Actions.Sim.clearMemory
        }
    }
    Menu {
        title: qsTr("&View")
        MenuItem {
            action: Actions.View.fullscreen
        }
        // Dynamic magic to mode switch!
    }
    Menu {
        title: qsTr("&Help")
        MenuItem {
            action: Actions.Help.about
        }
    }
}
