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
                        Component.onCompleted: checked = category.showDebugComponents
                        onCheckedChanged: category.showDebugComponents = checked
                        Connections {
                            target: category
                            function onShowDebugComponentsChanged() {
                                showDebug.checked = category.showDebugComponents
                            }
                        }
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
                        Component.onCompleted: checked = category.showMenuHotkeys
                        onCheckedChanged: category.showMenuHotkeys = checked
                        Connections {
                            target: category
                            function onShowChangeDialogChanged() {
                                hotkeyCheck.checked = category.showMenuHotkeys
                            }
                        }
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
                    columns: 2
                    CheckBox {
                        id: changeDialogCheck
                        Component.onCompleted: checked = category.showChangeDialog
                        onCheckedChanged: category.showChangeDialog = checked
                        Connections {
                            target: category
                            function onShowMenuHotkeysChanged() {
                                changeDialogCheck.checked = category.showChangeDialog
                            }
                        }
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
