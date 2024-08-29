import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// For PlatformDetector
import edu.pepp 1.0

//  Theme selection
RowLayout {
    id: root
    required property int buttonWidth
    required property var model

    Text {
        id: text
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
            if (Theme.isDirty) {
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
        text: "Delete"
        Layout.preferredWidth: buttonWidth

        //  Do not delete system themes
        enabled: !Theme.systemTheme
        onClicked: deleteLoader.item.open()
        palette {
            buttonText: !Theme.systemTheme ? root.palette.buttonText : root.palette.placeholderText
        }
    }
    Button {
        text: "Import"
        visible: !PlatformDetector.isWASM
        enabled: !PlatformDetector.isWASM
        Layout.preferredWidth: buttonWidth
        onClicked: importLoader.item.open()
    }
    Button {
        text: "Export"
        Layout.preferredWidth: buttonWidth

        visible: !PlatformDetector.isWASM
        //  Do not export system themes
        enabled: !Theme.systemTheme && !PlatformDetector.isWASM
        onClicked: exportLoader.item.open()
        palette {
            buttonText: !Theme.systemTheme ? root.palette.buttonText : root.palette.placeholderText
        }
    }


    /*FileDialog {
        id: exportDialog

        currentFolder: StandardPaths.standardLocations(
                           StandardPaths.AppConfigLocation)[0]
        fileMode: FileDialog.SaveFile
        title: "Export Theme"
        nameFilters: ["Pep Theme files (*.theme)"]
        defaultSuffix: "theme"
        selectedFile: Theme.name

        onAccepted: {
            Theme.exportTheme(decodeURIComponent(selectedFile))
        }
    }*/
    Loader {
        id: exportLoader
        Component.onCompleted: {
            const props = {
                "mode": "SaveFile",
                "title": "Export Theme",
                "nameFilters": ["Pep Theme files (*.theme)"],
                "selectedNameFilter_index": 0,
                "defaultSuffix": "theme",
                "selectedFile": Theme.name
            }

            if (PlatformDetector.isWASM) {
                console.warn("Export dialog not implemented for WASM.")
            } else {
                setSource("qrc:/ui/preferences/NativeFileDialog.qml", props)
            }
        }
        asynchronous: false
        Connections {
            target: exportLoader.item
            function onAccepted() {
                const d = decodeURIComponent(exportLoader.item.selectedFile)
                Theme.exportTheme(d)
            }
        }
    }
    Loader {
        id: importLoader
        Component.onCompleted: {
            const props = {
                "mode": "OpenFile",
                "title": "Import Theme",
                "nameFilters": ["Pep Theme files (*.theme)"],
                "selectedNameFilter_index": 0,
                "defaultSuffix": "theme"
            }

            if (PlatformDetector.isWASM) {
                console.warn("Import dialog not implemented for WASM.")
            } else {
                setSource("qrc:/ui/preferences/NativeFileDialog.qml", props)
            }
        }
        asynchronous: false
        Connections {
            target: importLoader.item
            function onAccepted() {
                const d = decodeURIComponent(importLoader.item.selectedFile)
                Theme.importTheme(d)
                //  Once new theme is imported, reset model to refresh screen.
                root.model.resetModel()
            }
        }
    }

    Loader {
        id: deleteLoader
        Component.onCompleted: {
            const props = {
                "title": "Delete Theme",
                "text": qsTr("Are you sure you want to delete this theme permanently?"),
                "standardButtons": Dialog.Ok | Dialog.Cancel
            }
            if (PlatformDetector.isWASM) {
                props["modal"] = true
                props["spacing"] = 5

                props["x"] = 0
                props["y"] = 0
                props["parent"] = root
                setSource("qrc:/ui/preferences/QMLMessageDialog.qml", props)
            } else
                setSource("qrc:/ui/preferences/NativeMessageDialog.qml", props)
        }
        asynchronous: false
        Connections {
            target: deleteLoader.item
            function onAccepted() {
                Theme.deleteTheme(themeId.currentText)
                //  Once current theme is deleted, default theme will be reloaded. Reset model to refresh screen.
                root.model.resetModel()
                themeId.currentIndex = themeId.find(Theme.name)
            }
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
                color: palette.text
                validator: RegularExpressionValidator {
                    regularExpression: /^[^<>:;,?"*|\\/]+$/
                }
            }
        }

        onAccepted: {
            Theme.copyTheme(fileName.text)
            themeId.currentIndex = themeId.find(Theme.name)
            color: palette.text
        }
    }
}
