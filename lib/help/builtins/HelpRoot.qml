import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "qrc:/qt/qml/edu/pepp/utils" as Comp

Item {
    id: root
    // Treat as read-only inputs. If changed, they should force updates to the combo boxes.
    property var architecture: 0
    property var abstraction: 0
    signal reloadFiguresRequested
    NuAppSettings {
        id: settings
    }

    onArchitectureChanged: {
        var idx = 0;
        for (var i = 0; i < architectureModel.count; i++) {
            if (architectureModel.get(i).value === architecture)
                idx = i;
        }
        architectureCombo.currentIndex = idx;
        architectureCombo.activated(idx);
    }
    onAbstractionChanged: {
        var idx = 0;
        for (var i = 0; i < abstractionModel.count; i++) {
            if (abstractionModel.get(i).value === abstraction)
                idx = i;
        }
        abstractionCombo.currentIndex = idx;
        abstractionCombo.activated(idx);
    }
    property var selected
    // Duplicated in GeneralCategoryDelegate. Must manually propogate changes between files.
    Component.onCompleted: {
        architectureModel.append({
            "key": "Pep/10",
            "value": Architecture.PEP10
        });
        architectureModel.append({
            "key": "Pep/9",
            "value": Architecture.PEP9
        });
        architectureModel.append({
            "key": "Pep/8",
            "value": Architecture.PEP8
        });
        architectureModel.append({
            "key": "RISC-V",
            "value": Architecture.RISCV
        });
        abstractionModel.append({
            "key": "ASMB5",
            "value": Abstraction.ASMB5
        });
        abstractionModel.append({
            "key": "ASMB3",
            "value": Abstraction.ASMB3
        });
        abstractionModel.append({
            "key": "ISA3",
            "value": Abstraction.ISA3
        });
        abstractionModel.append({
            "key": "MC2",
            "value": Abstraction.MC2
        });
        abstractionModel.append({
            "key": "OS4",
            "value": Abstraction.OS4
        });
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
                    textRole: "key"
                    valueRole: "value"
                    model: ListModel {
                        id: architectureModel
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
                    textRole: "key"
                    valueRole: "value"
                    model: ListModel {
                        id: abstractionModel
                    }
                }
            }
        }

        TreeView {
            id: treeView
            Layout.minimumWidth: textMetrics.width
            selectionModel: ItemSelectionModel {}
            Layout.fillHeight: true
            clip: true
            boundsMovement: Flickable.StopAtBounds
            model: FilteredHelpModel {
                id: helpModel
                model: HelpModel {}
                // Sane defaults
                architecture: architectureCombo.currentValue
                abstraction: abstractionCombo.currentValue
                showWIPItems: settings.general.showDebugComponents
                onAbstractionChanged: root.selected = treeView.index(0, 0)
                onArchitectureChanged: root.selected = treeView.index(0, 0)
                Component.onCompleted: {
                    helpModel.sort(0, Qt.AscendingOrder);
                    reloadFiguresRequested.connect(helpModel.model.onReloadFigures);
                }
            }

            delegate: TreeViewDelegate {
                id: treeDelegate
                width: TreeView.availableWidth
                // Default background does not fill entire delegate.
                background: Rectangle {
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: treeView.width

                    color: model.isExternal ? "red" : palette.base
                    border {
                        color: treeDelegate.current ? palette.highlight : "transparent"
                        width: 2
                    }
                }
                onCurrentChanged: {
                    if (current) {
                        makeActive();
                    }
                }
                onClicked: {
                    makeActive();
                    if (treeView.isExpanded(row)) {
                        treeView.collapseRecursively(row);
                    } else {
                        treeView.expandRecursively(row);
                    }
                }

                function makeActive() {
                    root.selected = treeDelegate.treeView.index(row, column);
                    treeDelegate.treeView.selectionModel.setCurrentIndex(root.selected, ItemSelectionModel.NoUpdate);
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
                    const abs = abstractionModel.get(abstractionCombo.currentIndex).value;
                    const arch = architectureModel.get(architectureCombo.currentIndex).value;
                    root.addProject(arch, abs, feats, texts, true);
                    if (tests && tests[0])
                        root.setCharIn(tests[0].input);
                    root.switchToMode(mode ?? "Editor");
                }

                function onRenameCurrentProject(name) {
                    root.renameCurrentProject(name);
                }

                ignoreUnknownSignals: true
            }
        }
    }

    signal addProject(int level, int abstraction, string feats, var text, bool reuse)

    signal renameCurrentProject(string name)

    signal setCharIn(string text)

    signal switchToMode(string mode)

    onSelectedChanged: {
        const props = helpModel.data(selected, HelpModel.Props);
        const url = helpModel.data(selected, HelpModel.Delegate);
        if (url !== undefined)
            contentLoader.setSource(url, props);
    }
}
