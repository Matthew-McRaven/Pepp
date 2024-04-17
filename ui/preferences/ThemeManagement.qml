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

  Text { id: text; text: "Current Theme" }
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
    text: "Copy";
    Layout.preferredWidth: buttonWidth
    onClicked: {
      copyDialog.open()
    }
  }
  Button {
    text: "Delete";
    Layout.preferredWidth: buttonWidth
    enabled: !Theme.systemTheme
    onClicked: deleteDialog.open()
  }
  Button {
    text: "Import";
    Layout.preferredWidth: buttonWidth
    onClicked: importDialog.open()
  }
  Button {
    text: "Export"
    Layout.preferredWidth: buttonWidth
    onClicked: exportDialog.open()
  }

  FileDialog {
    id: exportDialog

    currentFolder: StandardPaths.standardLocations(StandardPaths.DownloadLocation)[0]
    fileMode: FileDialog.SaveFile
    title: "Export Theme"
    nameFilters: ["Pep Theme files (*.theme)"]
    defaultSuffix: "theme"
    selectedFile: Theme.name

    //  Set dialog colors
    //Control.palette.windowText: Theme.container.foreground
    //Control.palette.base: Theme.container.background

    onAccepted: {
      Theme.exportTheme(decodeURIComponent(selectedFile))
    }
  }

  FileDialog {
    id: importDialog

    currentFolder: StandardPaths.standardLocations(StandardPaths.DownloadLocation)[0]
    fileMode: FileDialog.OpenFile
    title: "Import Theme"
    nameFilters: ["Pep Theme files (*.theme)"]
    selectedNameFilter.index: 0
    defaultSuffix: "theme"

    //  Set dialog colors
    //palette.text: Theme.container.foreground
    //palette.button: Theme.container.background
    //palette.window: Theme.container.background

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

    //  Set dialog colors
    palette.text: Theme.container.foreground
    palette.button: Theme.container.background
    palette.window: Theme.container.background

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

