import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

//  Theme selection
RowLayout {
  id: root
  required property int buttonWidth
  required property var model

  Text {
    id: text;
    text: "Current Theme"
    color: palette.windowText
  }
  ComboBox {
    id: themeId
    model: Theme.themes
    Component.onCompleted: {
      currentIndex = themeId.find(Theme.name)
    }

    onActivated: {
      Theme.selectTheme(themeId.currentText)

      //  When theme is changed, reset model to
      //  refresh screen.
      root.model.resetModel()
    }
  }
  Button {
    //  System themes can never have state change
    //  If non-system theme has changes, they must
    //  be saved before a copy can be made
    text: Theme.isDirty ? "Save" : "Copy"
    Layout.preferredWidth: buttonWidth
    onClicked: {
      if(Theme.isDirty) {
        //  If theme has change, changes must be saved first
        console.log("Save Theme")
        Theme.saveTheme()
      } else {
        console.log("Copy Theme")
        copyDialog.open()
      }
    }
  }
  Button {
    id: del
    text: "Delete";
    Layout.preferredWidth: buttonWidth

    //  Do not delete system themes
    enabled: !Theme.systemTheme
    onClicked: deleteDialog.open()
    palette {
      buttonText: !Theme.systemTheme
                  ? root.palette.buttonText
                  : root.palette.placeholderText
    }
  }
  Button {
    text: "Import";
    Layout.preferredWidth: buttonWidth
    onClicked: importDialog.open()
  }
  Button {
    text: "Export"
    Layout.preferredWidth: buttonWidth

    //  Do not export system themes
    enabled: !Theme.systemTheme
    onClicked: exportDialog.open()
    palette {
      buttonText: !Theme.systemTheme
                  ? root.palette.buttonText
                  : root.palette.placeholderText
    }
  }

  FileDialog {
    id: exportDialog

    currentFolder: StandardPaths.standardLocations(StandardPaths.AppConfigLocation)[0]
    fileMode: FileDialog.SaveFile
    title: "Export Theme"
    nameFilters: ["Pep Theme files (*.theme)"]
    defaultSuffix: "theme"
    selectedFile: Theme.name

    onAccepted: {
      Theme.exportTheme(decodeURIComponent(selectedFile))
    }
  }

  FileDialog {
    id: importDialog

    currentFolder: StandardPaths.standardLocations(StandardPaths.AppConfigLocation)[0]
    fileMode: FileDialog.OpenFile
    title: "Import Theme"
    nameFilters: ["Pep Theme files (*.theme)"]
    selectedNameFilter.index: 0
    defaultSuffix: "theme"

    //  Set dialog colors
    //palette.text: Theme.container.foreground

    onAccepted: {
      Theme.importTheme(decodeURIComponent(selectedFile))

      //  Once new theme is imported, reset model to
      //  refresh screen.
      root.model.resetModel()
    }
  }

  MessageDialog {
    id: deleteDialog
    title: "Delete Theme"
    text: qsTr("Are you sure you want to delete this theme permanently?")
    buttons: MessageDialog.Ok | MessageDialog.Cancel

    onButtonClicked: function (button, role) {
      switch (button) {
      case MessageDialog.Ok:
          Theme.deleteTheme(themeId.currentText)

          //  Once current theme is deleted, default
          //  theme will be reloaded. Reset model to
          //  refresh screen.
          root.model.resetModel()

          break;
      }
      themeId.currentIndex = themeId.find(Theme.name)
    }
  }

  Dialog {
    id: copyDialog
    title: "Copy Theme"
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    spacing: 5

    ColumnLayout {
      anchors.fill: parent
      Label {
        id: label
        text: "Color theme name:"
      }
      TextInput {
        id: fileName
        width: 100
        text: themeId.currentText + " (copy)"
        focus: true
        validator: RegularExpressionValidator { regularExpression: /^[^<>:;,?"*|\\/]+$/ }
      }
    }

    onAccepted: {
      Theme.copyTheme(fileName.text)
      themeId.currentIndex = themeId.find(Theme.name)
    }
  }
}

