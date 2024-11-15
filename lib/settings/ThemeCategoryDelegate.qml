import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import edu.pepp

Rectangle {
    property var category: undefined
    property int activeCategory: 0
    property int buttonWidth: 50
    id: root
    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width
    NuAppSettings {
        id: settings
    }

    PaletteFilterModel {
        id: paletteModel
        category: root.activeCategory
        sourceModel: PaletteModel {
            palette: settings.extPalette
        }
    }
    ColumnLayout {
        id: layout
        anchors.fill: parent
        GridLayout {
            columns: 4
            Label {
                text: "Current Palette: "
                Layout.alignment: Qt.AlignHCenter
            }
            ComboBox {
                id: comboBox
                model: PaletteManager {
                    id: onDisk
                }
                textRole: "display"
                valueRole: "path"
                ToolTip.visible: hovered
                ToolTip.text: currentValue ?? ""
                property bool isSystemTheme: onDisk.data(onDisk.index(
                                                             currentIndex, 0),
                                                         onDisk.isSystem)
            }
            Button {
                text: "Rename"
                Layout.minimumWidth: root.buttonWidth
                enabled: !comboBox.isSystemTheme
                palette {
                    buttonText: comboBox.isSystemTheme ? root.palette.placeholderText : root.palette.buttonText
                }
                onPressed: {
                    renameDialog.open()
                }
            }
            Item {}
            Button {
                id: copyButton
                //  System themes can never have state change
                //  If non-system theme has changes, they must be saved before a copy can be made
                text: comboBox.isSystemTheme ? "Copy" : "Save"
                Layout.minimumWidth: root.buttonWidth
                onPressed: {
                    if (comboBox.isSystemTheme) {
                        // Copy also creates the duplicate item for us!
                        const index = onDisk.copy(comboBox.index)
                        if (index != -1) {
                            comboBox.currentIndex = index
                            requestRename()
                        }
                    } else {
                        FileIO.save(comboBox.path,
                                    settings.extPalette.jsonString())
                    }
                }
                signal requestRename
            }
            Button {
                id: del
                text: "Delete"
                Layout.minimumWidth: root.buttonWidth
                enabled: !comboBox.isSystemTheme
                onClicked: {
                    const index = comboBox.currentIndex
                    if (index != -1) {
                        onDisk.deleteTheme(index)
                        comboBox.currentIndex = Math.min(index,
                                                         comboBox.count - 1)
                    }
                }
                palette {
                    buttonText: comboBox.isSystemTheme ? root.palette.placeholderText : root.palette.buttonText
                }
            }
            Button {
                text: "Import"
                Layout.minimumWidth: root.buttonWidth
                onClicked: importLoader.item.open()
            }
            Button {
                text: "Export"
                Layout.minimumWidth: root.buttonWidth
                // Now allows export of system themes. This makes it easier to keep theme files up-to-date.
                onClicked: exportLoader.item.open()
                palette {
                    buttonText: root.palette.buttonText
                }
            }
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Repeater {
                model: PaletteCategoryModel {}
                TabButton {
                    required property variant model
                    text: model.display
                    onClicked: {
                        root.activeCategory = Qt.binding(() => model.value)
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
            ListView {
                id: listView
                clip: true
                Layout.fillHeight: true
                Layout.minimumWidth: Math.max(
                                         100,
                                         contentItem.childrenRect.width + 5,
                                         childrenRect.width)
                focus: true
                focusPolicy: Qt.StrongFocus
                Keys.onUpPressed: listView.currentIndex = Math.max(
                                      0, listView.currentIndex - 1)
                Keys.onDownPressed: listView.currentIndex = Math.min(
                                        listView.count - 1,
                                        listView.currentIndex + 1)
                model: paletteModel
                delegate: BoxedText {
                    required property string display
                    required property var paletteRole
                    required property var paletteItem
                    name: display
                    bgColor: paletteItem?.background ?? "transparent"
                    fgColor: paletteItem?.foreground ?? "black"
                    font: paletteItem?.font ?? "Courier Prime"
                }
            }
            PaletteDetails {
                id: modifyArea
                ePalette: settings.extPalette
                paletteRole: listView.currentItem?.paletteRole
                paletteItem: listView.currentItem?.paletteItem
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
    Component.onCompleted: {
        copyButton.requestRename.connect(renameDialog.open)
    }
    Loader {
        id: exportLoader
        Component.onCompleted: {
            const props = {
                "mode": "SaveFile",
                "title": "Export Theme",
                "nameFilters": ["Pep Theme files (*.theme)"],
                "selectedNameFilter_index": 0,
                "defaultSuffix": "theme",
                "selectedFile": "default.theme"
            }

            if (PlatformDetector.isWASM) {
                setSource("qrc:/edu/pepp/settings/QMLFileDialog.qml", props)
            } else {
                setSource("qrc:/edu/pepp/settings/NativeFileDialog.qml", props)
            }
        }
        asynchronous: false
        Connections {
            target: exportLoader.item
            function onAccepted() {
                const path = decodeURIComponent(exportLoader.item.selectedFile)
                FileIO.save(path, settings.extPalette.jsonString())
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
                setSource("qrc:/edu/pepp/settings/NativeFileDialog.qml", props)
            }
        }
        asynchronous: false
        Connections {
            target: importLoader.item
            function onAccepted() {
                const model = comboBox.model
                const uri = decodeURIComponent(importLoader.item.selectedFile)
                const file = uri.replace("file:///", "")
                const index = model.importTheme(file)
                if (index != -1)
                    comboBox.currentIndex = index
            }
        }
    }
    Dialog {
        id: renameDialog
        title: "Rename Palette"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        spacing: 5
        ColumnLayout {
            anchors.fill: parent
            Label {
                id: label
                text: "Palette name:"
            }
            TextInput {
                id: name
                width: 100
                text: comboBox.displayText
                focus: true
                color: palette.text
                validator: RegularExpressionValidator {
                    regularExpression: /^[^<>:;,?"*|\\/]+$/
                }
            }
        }

        onAccepted: {
            const model = comboBox.model
            const index = model.index(comboBox.currentIndex, 0)
            model.setData(index, name.text, model.display)
        }
    }
}