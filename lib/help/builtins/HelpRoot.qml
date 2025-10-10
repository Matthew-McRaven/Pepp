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
    NuAppSettings {
        id: settings
    }
    function updateFilterCombo() {
        var comboIdx;
        for (var i = 0; i < filterModel.rowCount(); i++) {
            const idx = filterModel.index(i, 0);
            const arch = filterModel.data(idx, ProjectTypeModel.ArchitectureRole);
            const abs = filterModel.data(idx, ProjectTypeModel.LevelRole);
            //console.log(`Target is (${root.abstraction}, ${root.architecture}) found (${abs}, ${arch})`);
            if (root.abstraction === abs && root.architecture === arch) {
                comboIdx = i;
                break;
            }
        }
        //console.log(`Update filter combo to ${root.abstraction}, ${root.architecture}, idx is ${comboIdx}`);
        if (comboIdx !== undefined) {
            filterCombo.currentIndex = comboIdx;
            filterCombo.activated(comboIdx);
        }
    }
    Timer {
        id: filterComboCoalesce
        interval: 0
        repeat: false
        onTriggered: root.updateFilterCombo()
    }

    // When both are changed on the same frame, we would normally trigger twice.
    // Using a 0-length timer will coalesce these into a single update because we must enter the event loop to process the timer.
    onArchitectureChanged: filterComboCoalesce.restart()
    onAbstractionChanged: filterComboCoalesce.restart()

    // Make sure the drawer is always at least as wide as the text
    // There was an issue in WASM where the titles clipper the center area
    TextMetrics {
        id: textMetrics
        text: "       Computer Systems, 200th edition really"
    }

    Rectangle {
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            right: controlPanel.right
        }
        color: palette.base
        border.width: 1
        border.color: palette.shadow
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
                // Must not use use any form of: filterModel.get(<index>).value
                // We now have a a FilterModel with that matches everything ("Show All").
                // Previously, the value was always a single valid (arch, abstraction).
                // Now that this is no longer the case, this information needs to be tracked inside child components.
                Comp.DisableableComboBox {
                    id: filterCombo
                    textRole: "archAndAbs"
                    model: ProjectTypeFilterModel {
                        id: filterModel
                        edition: 0
                        showIncomplete: settings.general.showDebugComponents
                        showPartiallyComplete: settings.general.showDebugComponents
                        sourceModel: ProjectTypeModel {}
                    }
                }
            }
        }
        TreeView {
            id: treeView
            Layout.preferredWidth: textMetrics.width
            selectionModel: ItemSelectionModel {
                onCurrentIndexChanged: {
                    treeView.expandToIndex(currentIndex);
                    const props = helpModel.data(currentIndex, HelpModel.Props);
                    if (props && !("architecture" in props))
                        props["architecture"] = helpModel.architecture ?? Architecture.PEP10;
                    const url = helpModel.data(currentIndex, HelpModel.Delegate);
                    if (url !== undefined)
                        contentLoader.setSource(url, props);
                }
            }
            Layout.fillHeight: true
            clip: true
            boundsMovement: Flickable.StopAtBounds
            model: FilteredHelpModel {
                id: helpModel
                model: HelpModel {}
                // Sane defaults
                architecture: filterCombo.currentValue.architecture ?? 0
                abstraction: filterCombo.currentValue.abstraction ?? 0
                showWIPItems: settings.general.showDebugComponents
                onAbstractionChanged: {
                    const idx = treeView.index(0, 0);
                    treeView.selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect);
                }

                onArchitectureChanged: {
                    const idx = treeView.index(0, 0);
                    treeView.selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect);
                }
                Component.onCompleted: {
                    helpModel.sort(0, Qt.AscendingOrder);
                }
            }

            delegate: TreeViewDelegate {
                id: treeDelegate
                implicitWidth: textMetrics.width
                font: textMetrics.font

                Component.onCompleted: {
                    contentItem.textFormat = Text.MarkdownText;
                }
                // Default background does not fill entire delegate.
                background: Rectangle {
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: treeView.width

                    color: model.isExternal ? "red" : "transparent"
                    border {
                        color: treeDelegate.current ? palette.highlight : "transparent"
                        width: 2
                    }
                }

                //  Cannot override collapse or expand in onClicked, or it overrides default
                //  control behavior.
                onClicked: makeActive()

                function makeActive() {
                    const idx = treeDelegate.treeView.index(row, column);
                    treeDelegate.treeView.selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect);
                }
            }
        }
    }
    //  Background
    Rectangle {
        anchors {
            left: controlPanel.right
            top: parent.top
            right: parent.right
            bottom: parent.bottom
            topMargin: 0
            leftMargin: 10
            rightMargin: 20
        }
        color: palette.base
        border {
            color: palette.shadow
            width: 1
        }

        Flickable {
            id: contentFlickable
            ScrollBar.vertical: ScrollBar {}
            clip: true
            anchors {
                fill: parent
                margins: 0
            }
            Loader {
                id: contentLoader
                anchors.fill: parent
                Connections {
                    target: contentLoader.item

                    function onAddProject(arch, abs, feats, texts, mode, os, tests) {
                        root.addProject(arch, abs, feats, texts, true);
                        if (tests && tests[0])
                            root.setCharIn(tests[0].input);
                        root.switchToMode(mode ?? "Editor");
                    }

                    function onRenameCurrentProject(name) {
                        root.renameCurrentProject(name);
                    }

                    // Links starting with slug:// are interpeted as links from one help page to another.
                    function onNavigateTo(link) {
                        if (link.startsWith("slug:")) {
                            const idx = helpModel.indexFromSlug(link);
                            if (idx.valid)
                                treeView.selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect);
                        } else
                            Qt.openUrlExternally(link);
                    }

                    ignoreUnknownSignals: true
                }
                onLoaded: {
                    contentFlickable.contentHeight = Qt.binding(() => Math.max(contentFlickable.height, contentLoader?.item?.implicitHeight ?? 0));
                    contentFlickable.anchors.margins = (contentLoader?.item?.renderBG ?? true) ? 1 : 0;
                }
            }   //  Loader
        }   //  Flickable
    }   //  Rectangle - background

    signal addProject(int arch, int abstraction, string feats, var text, bool reuse)

    signal renameCurrentProject(string name)

    signal setCharIn(string text)

    signal switchToMode(string mode)
}
