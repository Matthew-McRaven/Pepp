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
                text: "Current Theme: "
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
                ToolTip.text: currentValue
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
            }
            Item {}
            Button {
                //  System themes can never have state change
                //  If non-system theme has changes, they must be saved before a copy can be made
                text: "Save"
                Layout.minimumWidth: root.buttonWidth
                enabled: !comboBox.isSystemTheme
                palette {
                    buttonText: comboBox.isSystemTheme ? root.palette.placeholderText : root.palette.buttonText
                }
            }
            Button {
                id: del
                text: "Delete"
                Layout.minimumWidth: root.buttonWidth
                enabled: !comboBox.isSystemTheme
                palette {
                    buttonText: comboBox.isSystemTheme ? root.palette.placeholderText : root.palette.buttonText
                }
            }
            Button {
                text: "Import"
                Layout.minimumWidth: root.buttonWidth
                onClicked: {

                }
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
}
