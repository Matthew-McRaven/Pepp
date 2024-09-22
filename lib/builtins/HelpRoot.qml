import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "qrc:/ui/components" as Comp

Item {
    id: root
    property var architecture: 0
    property var abstraction: 0
    onArchitectureChanged: {
        var idx = 0
        for (var i = 0; i < architectureModel.count; i++) {
            if (architectureModel.get(i).value === architecture)
                idx = i
        }
        architectureCombo.currentIndex = idx
        architectureCombo.activated(idx)
    }
    onAbstractionChanged: {
        var idx = 0
        for (var i = 0; i < abstractionModel.count; i++) {
            if (abstractionModel.get(i).value === abstraction)
                idx = i
        }
        abstractionCombo.currentIndex = idx
        abstractionCombo.activated(idx)
    }
    property var selected
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

    // Make sure the drawer is always at least as wide as the text
    // There was an issue in WASM where the titles clipper the center area
    TextMetrics {
        id: textMetrics
        text: "Computer Systems, 200th edition"
    }
    ColumnLayout {
        id: controlPanel
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        RowLayout {
            id: comboBoxes
            spacing: 0
            Column {
                Layout.fillWidth: false
                Label {
                    text: "Architecture"
                }
                Comp.DisableableComboBox {
                    id: architectureCombo
                    enabled: architecture === Architecture.NONE
                    textRole: "key"
                    valueRole: "value"
                    model: ListModel {
                        id: architectureModel
                    }
                    onCurrentIndexChanged: {
                        helpModel.architecture = Qt.binding(
                                    () => architectureModel.get(
                                        architectureCombo.currentIndex)?.value
                                    ?? Architecture.PEP10)
                    }
                }
            }
            Column {
                Layout.fillWidth: false
                Label {
                    text: "Abstraction"
                }
                Comp.DisableableComboBox {
                    id: abstractionCombo
                    enabled: architecture === Abstraction.NONE
                    textRole: "key"
                    valueRole: "value"
                    model: ListModel {
                        id: abstractionModel
                    }
                    onCurrentIndexChanged: {
                        helpModel.abstraction = Qt.binding(
                                    () => abstractionModel.get(
                                        abstractionCombo.currentIndex)?.value
                                    ?? Abstraction.ASMB5)
                    }
                }
            }
        }

        TreeView {
            id: treeView
            Layout.minimumWidth: textMetrics.width
            Layout.fillHeight: true
            clip: true
            model: FilteredHelpModel {
                id: helpModel
                model: HelpModel {}
                // Sane defaults
                abstraction: Abstraction.ASMB5
                architecture: Architecture.PEP10
                onAbstractionChanged: root.selected = treeView.index(0, 0)
                onArchitectureChanged: root.selected = treeView.index(0, 0)
            }

            delegate: TreeViewDelegate {
                id: treeDelegate
                width: TreeView.availableWidth
                // Default background does not fill entire delegate.
                background: Rectangle {
                    anchors.fill: parent
                    color: palette.base
                }
                onClicked: {
                    root.selected = treeDelegate.treeView.index(row, column)
                }
            }
        }
    }
    Flickable {
        id: contentFlickable
        anchors {
            left: controlPanel.right
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        Loader {
            id: contentLoader
            anchors.fill: parent
            Connections {
                target: contentLoader.item
                function onAddProject(feats, texts, mode, os, tests) {
                    const abs = abstractionModel.get(
                                  abstractionCombo.currentIndex).value
                    const arch = architectureModel.get(
                                   architectureCombo.currentIndex).value
                    root.addProject(arch, abs, feats, texts, true)
                    if (tests && tests[0])
                        root.setCharIn(tests[0].output)
                    root.switchToMode(mode ?? "Editor")
                }
                ignoreUnknownSignals: true
            }
        }
    }
    signal addProject(int level, int abstraction, string feats, var text, bool reuse)
    signal setCharIn(string text)
    signal switchToMode(string mode)
    onSelectedChanged: {
        const props = helpModel.data(selected, HelpModel.Props)
        const url = helpModel.data(selected, HelpModel.Delegate)
        if (url !== undefined)
            contentLoader.setSource(url, props)
    }
}
