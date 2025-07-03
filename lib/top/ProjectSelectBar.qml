import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    property bool requestHide: false
    property var currentProject: undefined
    property int currentProjectRow: pm.rowOf(currentProject)
    property var projectModel: pm
    property int popupX: 0
    property int popupY: 0
    // Propogate a status message from current project to main window.
    signal message(string message)
    clip: true
    visible: !requestHide && (pm.count > 0)
    implicitHeight: flickable.contentHeight
    NuAppSettings {
        id: settings
    }
    ProjectModel {
        id: pm
        function onAddProject(arch, level, feats, optTexts, reuse) {
            var proj = null;
            var cur = currentProject;
            settings.general.defaultArch = Number(arch);
            settings.general.defaultAbstraction = Number(level);
            // Attach a delegate to the project which can render its edit/debug modes. Since it is a C++ property,
            // binding changes propogate automatically.
            switch (Number(arch)) {
            case Architecture.PEP10:
                if (Number(level) === Abstraction.ISA3) {
                    if (cur && cur.architecture === Architecture.PEP10 && cur.abstraction === Abstraction.ISA3 && cur.isEmpty && reuse)
                        proj = cur;
                    else
                        proj = pm.pep10ISA();
                } else if (Number(level) === Abstraction.ASMB3) {
                    if (cur && cur.architecture === Architecture.PEP10 && cur.abstraction === Abstraction.ASMB3 && cur.isEmpty && reuse)
                        proj = cur;
                    else
                        proj = pm.pep10ASMB(Abstraction.ASMB3);
                } else if (Number(level) === Abstraction.ASMB5) {
                    if (cur && cur.architecture === Architecture.PEP10 && cur.abstraction === Abstraction.ASMB5 && cur.isEmpty && reuse)
                        proj = cur;
                    else
                        proj = pm.pep10ASMB(Abstraction.ASMB5);
                } else if (Number(level) === Abstraction.OS4) {
                    if (cur && cur.architecture === Architecture.PEP10 && cur.abstraction === Abstraction.OS4 && cur.isEmpty && reuse)
                        proj = cur;
                    else
                        proj = pm.pep10ASMB(Abstraction.OS4);
                }
                break;
            case Architecture.PEP9:
                if (Number(level) === Abstraction.ISA3) {
                    if (cur && cur.architecture === Architecture.PEP9 && cur.abstraction === Abstraction.ISA3 && cur.isEmpty && reuse)
                        proj = cur;
                    else
                        proj = pm.pep9ISA();
                } else if (Number(level) === Abstraction.ASMB5) {
                    if (cur && cur.architecture === Architecture.PEP9 && cur.abstraction === Abstraction.ASMB5 && cur.isEmpty && reuse)
                        proj = cur;
                    else
                        proj = pm.pep9ASMB();
                }
                break;
            }

            root.switchToProject(pm.rowOf(proj), true);
            if (optTexts === undefined || optTexts === null || optTexts === "")
                return onMarkActiveDirty(false);
            else if (typeof (optTexts) === "string")
                proj.set(level, optTexts);
            else {
                for (const list of Object.entries(optTexts))
                    proj.set(list[0], list[1]);
            }
            proj.overwriteEditors();
            onMarkActiveDirty(false);
        }
        function renameCurrentProject(string) {
            const row = pm.rowOf(currentProject);
            if (row == -1)
                return;
            const index = pm.index(row, 0);
            pm.setData(index, string, ProjectModel.NameRole);
        }
        function onMarkActiveDirty(dirtyValue) {
            const row = pm.rowOf(currentProject);
            if (row == -1)
                return;
            const index = pm.index(row, 0);
            pm.setData(index, dirtyValue, ProjectModel.DirtyRole);
        }
    }

    // Add Alt+# shortcuts to switch to correct tabs.
    Repeater {
        model: 10
        delegate: Item {
            Shortcut {
                property int index: modelData
                //%10 to ensure that Alt+0 maps to index==10
                sequences: [`Alt+${(index + 1) % 10}`]
                onActivated: switchToProject(index, true)
                enabled: index < pm.count
            }
        }
    }
    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: projectBar.width + addProjectButton.width
        contentHeight: projectBar.height
        // Use a row layout to handle proper sizing of tab bar and buttons.
        Row {
            TabBar {
                id: projectBar
                Layout.fillWidth: true
                Layout.fillHeight: true
                onCurrentIndexChanged: root.setCurrentProject(currentIndex)
                Repeater {
                    model: pm
                    anchors.fill: parent
                    delegate: TabButton {
                        id: tabButton
                        required property string name
                        required property string description
                        required property string path
                        required property int row
                        required property bool isDirty
                        text: `${tabButton.name} ${tabButton.isDirty ? " *" : ''}<br>${tabButton.description}`
                        ToolTip.text: path
                        hoverEnabled: true
                        ToolTip.visible: hovered && path
                        font {
                            family: menuFont.font.family
                            pixelSize: Math.min(16, menuFont.font.pixelSize)
                            italic: tabButton.isDirty
                        }
                        width: Math.max(200, projectSelect.width / 4, implicitContentWidth)
                        Button {
                            text: "X"
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            height: Math.max(20, parent.height - 2)
                            width: height
                            onClicked: root.closeProject(tabButton.row, false)
                        }
                    }
                }
            }
            Button {
                id: addProjectButton
                text: "+"
                Layout.fillHeight: true
                width: plusTM.width
                font: menuFont.font
                height: projectBar.height // Border may clip into main area if unset.
                TextMetrics {
                    id: plusTM
                    font: addProjectButton.font
                    text: `+${' '.repeat(10)}`
                }
                background: Rectangle {
                    color: "transparent"
                    border.color: addProjectButton.hovered ? palette.link : "transparent"
                    border.width: 2
                    radius: 4
                }
                onClicked: {
                    pm.onAddProject(root.currentProject.architecture, root.currentProject.abstraction, "");
                    root.switchToProject(pm.count - 1);
                }
            }
        }
    }
    Connections {
        target: root.currentProject ?? null
        function onObjectCodeTextChanged() {
            pm.onMarkActiveDirty(true);
        }
        function onUserAsmTextChanged() {
            pm.onMarkActiveDirty(true);
        }
        ignoreUnknownSignals: true
    }

    Dialog {
        id: unsavedDialog
        property int index: -1
        property bool force: false
        modal: true
        title: "Unsaved Changes"
        standardButtons: force ? (Dialog.Save | Dialog.Discard) : (Dialog.Save | Dialog.Discard | Dialog.Cancel)
        visible: false
        height: 240
        width: 320
        onAccepted: {
            const didSave = pm.onSave(unsavedDialog.index);
            if (didSave)
                root.discardProject(unsavedDialog.index);
        }

        onDiscarded: {
            root.discardProject(unsavedDialog.index);
        }

        contentItem: Label {
            text: "You have unsaved changes.\nWhat would you like to do?"
            wrapMode: Text.Wrap
            padding: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    function setCurrentProject(index) {
        if (root.currentProject?.message !== undefined) {
            root.currentProject.message.disconnect(root.message);
        }
        const proj = pm.data(pm.index(index, 0), ProjectModel.ProjectPtrRole);
        if (proj) {
            root.currentProject = Qt.binding(() => proj);
            projectBar.currentIndex = index;
        }
        if (root.currentProject?.message)
            root.currentProject.message.connect(root.message);
    }

    function switchToProject(index, force) {
        var needsManualUpdate = (force ?? false) && projectBar.currentIndex === index;
        if (needsManualUpdate)
            setCurrentProject(index);
        else
            projectBar.currentIndex = index;
    }

    // Try to save the contents of the project it it is dirty.
    // If force is true, no cancel button is provided.
    function closeProject(index, force) {
        const dirty = pm.data(pm.index(index, 0), ProjectModel.DirtyRole);
        if (dirty) {
            root.switchToProject(index, true);
            unsavedDialog.index = index;
            unsavedDialog.force = force;
            unsavedDialog.x = root.popupX;
            unsavedDialog.y = root.popupY;
            unsavedDialog.visible = true;
            unsavedDialog.open();
        } else
            discardProject(index);
    }
    // Unconditionally discard the project at the given index.
    function discardProject(index) {
        pm.removeRows(index, 1);
        if (pm.rowCount() === 0)
            return;
        else if (index < pm.rowCount())
            switchToProject(index, true);
        else {
            switchToProject(pm.rowCount() - 1, true);
        }
    }
}
