import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import edu.pepp 1.0

Item {
    id: root
    property var category: undefined
    Rectangle {
        id: bg
        color: palette.base
        anchors {
            fill: parent
            margins: border.width
        }
        border {
            color: palette.text
            width: 1
        }
    }
    ScrollView {
        id: flickable
        anchors {
            fill: parent
            margins: 2 * bg.border.width
            leftMargin: 4 * anchors.margins
        }
        ScrollBar.vertical.policy: flickable.contentHeight
                                   > flickable.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: flickable.contentWidth
                                     > flickable.width ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded

        ColumnLayout {
            GroupBox {
                Layout.fillWidth: true
                label: GroupBoxLabel {
                    text: qsTr("Project Defaults")
                    backgroundColor: bg.color
                }
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    ComboBox {
                        id: defaultArchCombo
                        textRole: "key"
                        valueRole: "value"
                        model: ListModel {
                            id: architectureModel
                        }
                        onActivated: {
                            const idx = defaultArchCombo.currentIndex
                            const value = architectureModel.get(idx).value
                            category.defaultArch = value
                        }
                        Component.onCompleted: {
                            currentIndex = indexOfValue(category.defaultArch)
                        }
                        Connections {
                            target: category

                            function onDefaultArchChanged() {
                                defaultArchCombo.currentIndex = defaultArchCombo.indexOfValue(
                                            category.defaultArch)
                            }
                        }
                    }
                    Label {
                        text: qsTr("Default project architecture")
                    }
                    ComboBox {
                        id: defaultAbsCombo
                        textRole: "key"
                        valueRole: "value"
                        model: ListModel {
                            id: abstractionModel
                        }
                        onActivated: {
                            const idx = defaultAbsCombo.currentIndex
                            const value = abstractionModel.get(idx).value
                            category.defaultAbstraction = value
                        }
                        Component.onCompleted: {
                            currentIndex = indexOfValue(
                                        category.defaultAbstraction)
                        }
                        Connections {
                            target: category

                            function onDefaultAbstractionChanged() {
                                defaultAbsCombo.currentIndex = defaultAbsCombo.indexOfValue(
                                            category.defaultAbstraction)
                            }
                        }
                    }
                    Label {
                        text: qsTr("Default project abstraction")
                    }
                    CheckBox {
                        id: showDebug
                        checked: category.showDebugComponents
                        onClicked: category.showDebugComponents = showDebug.checked
                    }
                    Label {
                        text: "Show work-in-progress UI components"
                    }
                }
            }
            GroupBox {
                label: GroupBoxLabel {
                    text: qsTr("Menu Options")
                    backgroundColor: bg.color
                }
                Layout.fillWidth: true
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    SpinBox {
                        from: 0
                        to: 10
                        value: category.maxRecentFiles
                        onValueModified: {
                            category.maxRecentFiles = value
                        }
                    }
                    Label {
                        text: "Max recent files"
                    }
                    CheckBox {
                        id: hotkeyCheck
                        checked: category.showMenuHotkeys
                    }
                    Label {
                        text: "Show hotkeys in menu entries"
                    }
                }
            }
            GroupBox {
                label: GroupBoxLabel {
                    text: qsTr("Update Settings")
                    backgroundColor: bg.color
                }
                Layout.fillWidth: true
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    CheckBox {
                        id: changeDialogCheck
                        checked: category.showChangeDialog
                        onClicked: category.showChangeDialog = changeDialogCheck.checked
                    }
                    Label {
                        text: "Show \"What's Changed\" dialog on update"
                    }
                    Button {
                        Layout.columnSpan: 2
                        Layout.maximumWidth: 2 * implicitWidth
                        text: qsTr("Check GitHub for updates")
                        onClicked: {
                            Qt.openUrlExternally(
                                        "https://github.com/Matthew-McRaven/Pepp/releases")
                        }
                    }
                }
            }
            GroupBox {
                visible: category.showDebugComponents
                enabled: category.showDebugComponents
                label: GroupBoxLabel {
                    text: qsTr("App Developer Options")
                    backgroundColor: bg.color
                }
                Layout.fillWidth: true
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    CheckBox {
                        id: allowExternFigures
                        visible: !PlatformDetector.isWASM
                        enabled: !PlatformDetector.isWASM
                        checked: category.allowExternalFigures
                        onClicked: category.allowExternalFigures = allowExternFigures.checked
                    }
                    Label {
                        visible: !PlatformDetector.isWASM
                        enabled: !PlatformDetector.isWASM
                        text: "Use external figures"
                    }

                    TextField {
                        id: externFigurePath
                        visible: !PlatformDetector.isWASM
                        enabled: category.allowExternalFigures
                                 && !PlatformDetector.isWASM
                        Layout.fillWidth: true
                        Layout.minimumWidth: 160
                        placeholderText: "/path/to/books/directory/"
                        text: category.externalFigureDirectory
                    }

                    Button {
                        visible: !PlatformDetector.isWASM
                        enabled: category.allowExternalFigures
                                 && !PlatformDetector.isWASM
                        text: qsTr("Choose directory...")
                        onClicked: figureLoader.item.open()
                        Connections {
                            target: figureLoader.item

                            function onAccepted() {
                                let path = decodeURIComponent(
                                        figureLoader.item.selectedFolder)
                                path = path.replace(/^(file:\/{2})/, "")
                                category.externalFigureDirectory = path
                            }
                        }

                        Loader {
                            id: figureLoader
                            Component.onCompleted: {
                                const props = {
                                    "title": "Select Figure Directory"
                                }
                                if (!PlatformDetector.isWASM) {
                                    setSource("qrc:/edu/pepp/settings/NativeFolderDialog.qml",
                                              props)
                                }
                            }
                            asynchronous: false
                        }
                    }
                }
            }
            Item {
                Layout.fillHeight: true
            }
        }
    }
    // Duplicated from HelpRoot.qml. Must manually propogate changes between files.
    Component.onCompleted: {
        architectureModel.append({
            "key": "Pep/10",
            "value": Architecture.PEP10
        })
        architectureModel.append({
            "key": "Pep/9",
            "value": Architecture.PEP9
        })
        architectureModel.append({
            "key": "Pep/8",
            "value": Architecture.PEP8
        })
        architectureModel.append({
            "key": "RISC-V",
            "value": Architecture.RISCV
        })
        abstractionModel.append({
            "key": "ASMB5",
            "value": Abstraction.ASMB5
        })
        abstractionModel.append({
            "key": "ASMB3",
            "value": Abstraction.ASMB3
        })
        abstractionModel.append({
            "key": "ISA3",
            "value": Abstraction.ISA3
        })
        abstractionModel.append({
            "key": "MC2",
            "value": Abstraction.MC2
        })
        abstractionModel.append({
            "key": "OS4",
            "value": Abstraction.OS4
        })
    }
}
