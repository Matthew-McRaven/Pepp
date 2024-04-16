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

    onAccepted: {
      Theme.importTheme(decodeURIComponent(selectedFile))
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

    palette.text: Theme.container.foreground
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

