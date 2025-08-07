import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "qrc:/qt/qml/edu/pepp/utils" as Comp

Item {
    id: root
    property real topOffset: 0
    // Treat as read-only inputs. If changed, they should force updates to the combo boxes.
    property var architecture: 0
    property var abstraction: 0
    signal reloadFiguresRequested
    NuAppSettings {
        id: settings
    }

    onArchitectureChanged: {
        var idx = 0;
        for (var i = 0; i < filterModel.count; i++) {
            const v = filterModel.get(i).value;
            if (root.abstraction === v.abstraction && root.architecture === v.architecture)
                idx = i;
        }
        filterCombo.currentIndex = idx;
        filterCombo.activated(idx);
    }
    onAbstractionChanged: {
        var idx = 0;
        for (var i = 0; i < filterModel.count; i++) {
            const v = filterModel.get(i).value;
            if (root.abstraction === v.abstraction && root.architecture === v.architecture)
                idx = i;
        }
        filterCombo.currentIndex = idx;
        filterCombo.activated(idx);
    }
    property var selected
    function make(arch, abs) {
        return {
            "architecture": arch,
            "abstraction": abs
        };
    }

    // Duplicated in GeneralCategoryDelegate. Must manually propogate changes between files.
    Component.onCompleted: {
        filterModel.append({
            "key": "Pep/10, ISA3",
            "value": make(Architecture.PEP10, Abstraction.ISA3)
        });
        filterModel.append({
            "key": "Pep/10, ASMB3",
            "value": make(Architecture.PEP10, Abstraction.ASMB3)
        });
        filterModel.append({
            "key": "Pep/10, ASMB5",
            "value": make(Architecture.PEP10, Abstraction.ASMB5)
        });
        filterModel.append({
            "key": "Pep/10, OS4",
            "value": make(Architecture.PEP10, Abstraction.OS4)
        });
        filterModel.append({
            "key": "Pep/10, MC2",
            "value": make(Architecture.PEP10, Abstraction.MC2)
        });
        filterModel.append({
            "key": "RISC-V, ASMB3",
            "value": make(Architecture.RISCV, Abstraction.ASMB3)
        });
        filterModel.append({
            "key": "Pep/9, ISA3",
            "value": make(Architecture.PEP9, Abstraction.ISA3)
        });
        filterModel.append({
            "key": "Pep/9, ASMB5",
            "value": make(Architecture.PEP9, Abstraction.ASMB5)
        });
        filterModel.append({
            "key": "Pep/9, OS4",
            "value": make(Architecture.PEP9, Abstraction.OS4)
        });
        filterModel.append({
            "key": "Pep/9, MC2",
            "value": make(Architecture.PEP9, Abstraction.MC2)
        });
    }

    // Make sure the drawer is always at least as wide as the text
    // There was an issue in WASM where the titles clipper the center area
    TextMetrics {
        id: textMetrics
        text: "       Computer Systems, 200th edition really"
    }
    ColumnLayout {
        id: controlPanel
        width: textMetrics.width
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            topMargin: -root.topOffset
        }
        RowLayout {
            id: comboBoxes
            spacing: 0
            Column {
                Layout.fillWidth: false
                Label {
                    text: "Filter to: "
                }
                Comp.DisableableComboBox {
                    id: filterCombo
                    textRole: "key"
                    valueRole: "value"
                    model: ListModel {
                        id: filterModel
                    }
                }
            }
        }
        TreeView {
            id: treeView
            Layout.preferredWidth: textMetrics.width
            selectionModel: ItemSelectionModel {}
            Layout.fillHeight: true
            clip: true
            boundsMovement: Flickable.StopAtBounds
            model: FilteredHelpModel {
                id: helpModel
                model: HelpModel {}
                // Sane defaults
                architecture: filterCombo.currentValue.architecture
                abstraction: filterCombo.currentValue.abstraction
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
                implicitWidth: textMetrics.width

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
                font: textMetrics.font
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
            topMargin: 0
            leftMargin: 20
            rightMargin: 20
            bottomMargin: 20
        }
        ScrollBar.vertical: ScrollBar {}
        clip: true
        Loader {
            id: contentLoader
            anchors.fill: parent
            Connections {
                target: contentLoader.item

                function onAddProject(feats, texts, mode, os, tests) {
                    const abs = filterModel.get(filterCombo.currentIndex).value.abstraction;
                    const arch = filterModel.get(filterCombo.currentIndex).value.architecture;
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
            onLoaded: {
                // Offset by some small amount to disappear scrollbar when content is not large enough.
                const height = Math.max(contentFlickable.height - 1, contentLoader?.item?.implicitHeight ?? 0);
                contentFlickable.contentHeight = Qt.binding(() => height);
            }
        }
    }

    signal addProject(int level, int abstraction, string feats, var text, bool reuse)

    signal renameCurrentProject(string name)

    signal setCharIn(string text)

    signal switchToMode(string mode)

    onSelectedChanged: {
        const props = helpModel.data(selected, HelpModel.Props);
        if (props)
            props["architecture"] = helpModel.architecture ?? Architecture.PEP10;
        const url = helpModel.data(selected, HelpModel.Delegate);
        if (url !== undefined)
            contentLoader.setSource(url, props);
    }
}
