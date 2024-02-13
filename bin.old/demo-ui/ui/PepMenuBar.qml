import QtQuick
import QtQuick.Controls

MenuBar {
  spacing: 10

  property var aboutDialog
  //property var textEditor
  //property SaveChangesDialog saveChangesDialog

  //  File Menu
  Menu {
     id: fileMenu
     title: qsTr("File")

     // Usually we'd set a NoFocus policy on controls so that we
     // can be sure that shortcuts are only activated when the canvas
     // has focus (i.e. no popups are open), but doing so on MenuItems
     // would prevent them from being navigable with the arrow keys.
     // Instead, we just force focus on the canvas when a menu closes.
     // Note that we need to check if the canvas exists, as the menu
     // can close when e.g. creating a new project without having one
     // already open.
     //onClosed: if (canvas) canvas.forceActiveFocus()

    MenuItem {
      objectName: "newMenuButton"
      text: qsTr("New")
      //onTriggered: textEditor.open()
      //onTriggered: saveChangesDialog.doIfChangesSavedOrDiscarded(function() { newProjectPopup.open() }, true)
    }

    MenuItem {
      objectName: "openMenuItem"
      text: qsTr("Open")
      onTriggered: openFileDialog.open()
    }

    MenuSeparator {}

   /*
    Menu {
      id: recentFilesSubMenu
      objectName: "recentFilesSubMenu"
      title: qsTr("Recent Files")
      // This can use LayoutGroup if it's ever implemented: https://bugreports.qt.io/browse/QTBUG-44078
      width: 400
      enabled: recentFilesInstantiator.count > 0

      onClosed: canvas.forceActiveFocus()

      Instantiator {
        id: recentFilesInstantiator
        objectName: "recentFilesInstantiator"
        //model: settings.recentFiles
        delegate: MenuItem {
          // We should elide on the right when it's possible without losing the styling:
          // https://bugreports.qt.io/browse/QTBUG-70961
          objectName: text + "MenuItem"
          //text: settings.displayableFilePath(modelData)
          onTriggered: saveChangesDialog.doIfChangesSavedOrDiscarded(function() {
            // If we load the project immediately, it causes the menu items to be removed immediately,
            // which means the Menu that owns them will disconnect from the triggered() signal of the
            // menu item, resulting in the menu not closing:
            //
            // https://github.com/mitchcurtis/slate/issues/128
            //
            // For some reason, this doesn't happen with native menus, possibly because
            // the removal and insertion is delayed there.
            Qt.callLater(function() {
               loadProject(modelData)
            })
          }, true)
        }

        onObjectAdded: (index, object) => recentFilesSubMenu.insertItem(index, object)
        onObjectRemoved: (index, object) => recentFilesSubMenu.removeItem(object)
      }

      MenuSeparator {}
    }

    MenuItem {
      objectName: "clearRecentFilesMenuItem"
      //: Empty the list of recent files in the File menu.
      text: qsTr("Clear Recent Files")
      //onTriggered: settings.clearRecentFiles()
    }

    MenuSeparator {}
*/
    MenuItem {
      objectName: "saveMenuItem"
      text: qsTr("Save Source")
      //enabled: project && project.canSave
      //onTriggered: //projectManager.saveOrSaveAs()
    }

    MenuSeparator {}

    MenuItem {
      objectName: "saveSourceAsMenuItem"
      text: qsTr("Save Source As")
      //enabled: project && project.loaded
      onTriggered: {
        saveAsDialog.type = 0
        saveAsDialog.open()
        //projectManager.saveOrSaveAs()
      }
    }

    MenuItem {
      objectName: "saveObjectAsMenuItem"
      text: qsTr("Save Object Code As")
      //enabled: project && project.loaded
      onTriggered: {
        saveAsDialog.type = 1
        saveAsDialog.open()
        //projectManager.saveOrSaveAs()
      }
    }

    MenuItem {
      objectName: "saveListingAsMenuItem"
      text: qsTr("Save Listing As")
      //enabled: project && project.loaded
      onTriggered: {
        saveAsDialog.type = 2
        saveAsDialog.open()
        //projectManager.saveOrSaveAs()
      }
    }

    MenuSeparator {}

    MenuItem {
      objectName: "quitMenuItem"
      text: qsTr("Exit")
      onTriggered: Qt.quit()
      //onTriggered: saveChangesDialog.doIfChangesSavedOrDiscarded(function() { Qt.quit() })
    }
  }

  //  Edit Menu
  Menu {
    id: editMenu
    title: qsTr("Edit")

    onClosed: canvas.forceActiveFocus()

    MenuItem {
      objectName: "undoMenuItem"
      text: qsTr("Undo")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "redoMenuItem"
      text: qsTr("Redo")
      //onTriggered: aboutDialog.open()
    }
    MenuSeparator {}

    MenuItem {
      objectName: "cutMenuItem"
      text: qsTr("Cut")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "copyMenuItem"
      text: qsTr("Copy")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "pasteMenuItem"
      text: qsTr("Paste")
      //onTriggered: aboutDialog.open()
    }
    MenuSeparator {}
    MenuItem {
      objectName: "fontSelectMenuItem"
      text: qsTr("Select Font")
      onTriggered: fontDialog.open();
      /*{
        fontDialog.open();
        //window.currentFont = fontDialog.currentFont;
        console.log(fontDialog.currentFont);
      }*/
    }
    MenuItem {
      objectName: "fontResetMenuItem"
      text: qsTr("Font Reset")
      /*onTriggered: {
        //window.currentFont = window.defaultFont
        console.log(window.currentFont);
      }*/
    }
  }

  //  Build Menu
  Menu {
    id: buildMenu
    title: qsTr("Build")

    onClosed: canvas.forceActiveFocus()

    MenuItem {
      objectName: "assembleMenuItem"
      text: qsTr("Assemble Source")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "loadObjectMenuItem"
      text: qsTr("Load object Code")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "executeMenuItem"
      text: qsTr("Execute")
      //onTriggered: aboutDialog.open()
    }
    MenuSeparator {}

    MenuItem {
      objectName: "runSourceMenuItem"
      text: qsTr("Run Source")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "runObjectMenuItem"
      text: qsTr("Run Object")
      //onTriggered: aboutDialog.open()
    }
  }

  //  Debug Menu
  Menu {
    id: debugMenu
    title: qsTr("Debug")

    onClosed: canvas.forceActiveFocus()

    MenuItem {
      objectName: "startDebugMenuItem"
      text: qsTr("Start Debugging")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "debugObjectMenuItem"
      text: qsTr("Start Debugging Object")
      //onTriggered: aboutDialog.open()
    }
    MenuItem {
      objectName: "debugLoaderMenuItem"
      text: qsTr("Start Debugging Loader")
      //onTriggered: aboutDialog.open()
    }
  }

  //  Help Menu
  Menu {
    id: helpMenu
    title: qsTr("Help")

    //onClosed: canvas.forceActiveFocus()

    MenuItem {
      objectName: "aboutMenuItem"
      text: qsTr("About Pep10")
      onTriggered: aboutDialog.open()
    }
  }
}
