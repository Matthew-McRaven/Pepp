/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
// If you want a plugin loaded, you better import it here.
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import Qt.labs.platform as P// Make sure app fails to start if this is missing.
import Qt.labs.qmlmodels as M// "
import "qrc:/qt/qml/edu/pepp/help/about" as About
import "qrc:/qt/qml/edu/pepp/project" as Project
import "qrc:/qt/qml/edu/pepp/top" as Top
import "qrc:/qt/qml/edu/pepp/settings" as AppSettings
import "qrc:/qt/qml/edu/pepp/help/builtins" as Builtins
import "qrc:/qt/qml/edu/pepp/menu" as Menu
import edu.pepp 1.0
import com.kdab.dockwidgets 2 as KDDW

ApplicationWindow {
    id: window
    width: 1920
    height: 1080
    visible: true
    title: qsTr("Pepp IDE")
    property alias currentProjectRow: projectSelect.currentProjectRow
    property alias currentProject: projectSelect.currentProject
    property alias pm: projectSelect.projectModel
    // Compiler keeps identifying this as a singleton, but it is not.
    NuAppSettings {
        id: settings
    }

    //  Set palette in parent. Inherited by all children
    /*  See https://doc.qt.io/qt-6/qml-qtquick-colorgroup.html for color explanation*/
    palette {
        alternateBase: settings.extPalette.alternateBase.background
        base: settings.extPalette.base.background
        text: settings.extPalette.base.foreground
        button: settings.extPalette.button.background
        buttonText: settings.extPalette.button.foreground
        highlight: settings.extPalette.highlight.background
        highlightedText: settings.extPalette.highlight.foreground
        toolTipBase: settings.extPalette.tooltip.background
        toolTipText: settings.extPalette.tooltip.foreground
        window: settings.extPalette.window.background
        windowText: settings.extPalette.window.foreground
        accent: settings.extPalette.accent.background
        light: settings.extPalette.light.background
        midlight: settings.extPalette.midlight.background
        mid: settings.extPalette.mid.background
        dark: settings.extPalette.dark.background
        shadow: settings.extPalette.shadow.background
        link: settings.extPalette.link.foreground
        linkVisited: settings.extPalette.linkVisited.foreground
        brightText: settings.extPalette.brightText.foreground
        placeholderText: settings.extPalette.placeholderText.foreground
        //  Colors when control is disabled. Overrides normal palette
        disabled {
            highlight: settings.extPalette.window.background
            buttonText: settings.extPalette.base.foreground
            text: settings.extPalette.shadow.foreground
            button: settings.extPalette.base.background
        }
    }

    // Used to expose actions to inner area.
    property var actionRef: actions
    property string mode: "welcome"
    function setProjectCharIn(charIn) {
        if (currentProject)
            currentProject.charIn = charIn;
    }

    Component.onCompleted: {
        // Allow welcome mode to create a new project, and switch to it on creation.
        welcome.addProject.connect(pm.onAddProject);
        welcome.addProject.connect(() => sidebar.switchToMode("Editor"));
        help.addProject.connect(pm.onAddProject);
        help.switchToMode.connect(sidebar.switchToMode);
        help.setCharIn.connect(i => setProjectCharIn(i));
        help.renameCurrentProject.connect(pm.renameCurrentProject);

        actions.edit.prefs.triggered.connect(preferencesDialog.open);
        actions.help.about.triggered.connect(aboutDialog.open);
        actions.view.fullscreen.triggered.connect(onToggleFullScreen);
        actions.file.save.triggered.connect(() => {
            syncEditors();
            pm.onSave(currentProjectRow);
        });
        actions.appdev.reloadFigures.triggered.connect(help.reloadFiguresRequested);
        projectSelect.message.connect(message);
        //message.connect(text => footer.text = text);
        //message.connect(() => messageTimer.restart());
        //messageTimer.restart();
        sidebar.switchToMode("Welcome");
        pm.rowCountChanged.connect(noOpenProjectCheck);
    }

    // Provide a default font for menu items.
    FontMetrics {
        id: menuFont
    }
    function syncEditors() {
        const loader = delegateRepeater.itemAt(innerLayout.currentIndex);
        if (loader.item)
            loader.item.syncEditors();
    }
    function noOpenProjectCheck() {
        if (pm.rowCount() === 0)
            sidebar.switchToMode("Welcome");
    }

    Menu.Actions {
        id: actions
        project: window.currentProject
        window: window
        settings: settings
    }

    signal message(string message)
    /*footer: Label {
        anchors.leftMargin: 10
        text: "test message"
        Timer {
            id: messageTimer
            interval: 10000
            onTriggered: window.footer.text = ""
        }
    }*/

    Top.ToolBar {
        id: toolbar
        visible: !(window.mode === "welcome" || window.mode === "help")
        anchors.top: parent.top
        anchors.left: sidebar.right
        anchors.right: parent.right
        actions: actions
    }

    Top.ProjectSelectBar {
        id: projectSelect
        requestHide: window.mode === "welcome" || window.mode === "help"
        anchors.right: parent.right
        anchors.left: sidebar.right
        anchors.top: toolbar.bottom
        popupX: mainArea.x
        popupY: mainArea.y
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: sidebar.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: Qt.darker(palette.shadow, 1.25)
            }
            GradientStop {
                position: 1.0
                color: Qt.lighter(palette.shadow, 1.15)
            }
        }
    }

    Top.SideBar {
        id: sidebar
        modesModel: window.currentProject ? window.currentProject.modes() : undefined
        anchors {
            top: toolbar.bottom
            bottom: parent.bottom
            left: parent.left
        }
        width: 100

        onModeChanged: function (text) {
            window.mode = text.toLowerCase();
        }
    }

    StackLayout {
        id: mainArea
        anchors.top: projectSelect.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: sidebar.right
        Top.Welcome {
            id: welcome
            topOffset: toolbar.height
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        Builtins.HelpRoot {
            id: help
            abstraction: currentProject?.abstraction ?? Abstraction.NONE
            architecture: currentProject?.architecture ?? Architecture.NONE
            topOffset: toolbar.height
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        StackLayout {
            id: innerLayout
            currentIndex: projectSelect.currentProjectRow
            Layout.fillHeight: true
            Layout.fillWidth: true
            Repeater {
                id: delegateRepeater
                model: pm
                delegate: Loader {
                    id: projectLoader
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Component.onCompleted: {
                        const delegate = model.project.delegatePath();
                        setSource(delegate, {
                            "project": model.project,
                            "mode": Qt.binding(() => window.mode),
                            "actions": window.actionRef,
                            "isActive": false
                        });
                        // Do not attempt to put docking widgets in main area until size is non-0.
                        // Instead, listen for updates in attemptDock, and perform docking as soon as we have real sizes.
                        widthChanged.connect(attemptDock);
                        heightChanged.connect(attemptDock);
                    }
                    function attemptDock() {
                        if (height == 0 || width == 0) {} else if (item !== null && item.needsDock) {
                            item.dock();
                            // Once docked, stop handling docking attempts for each resize.
                            widthChanged.disconnect(attemptDock);
                            heightChanged.disconnect(attemptDock);
                        }
                    }
                    onLoaded: {
                        projectLoader.item.isActive = Qt.binding(() => model.row === projectSelect.currentProjectRow);
                    }

                    Connections {
                        target: projectLoader.item
                        enabled: model.row == projectSelect.currentProjectRow
                        function onRequestModeSwitchTo(mode) {
                            sidebar.switchToMode(mode);
                        }
                    }
                }
            }
        }
        Component.onCompleted: {
            window.modeChanged.connect(onModeChanged);
            onModeChanged();
        }
        function onModeChanged() {
            switch (window.mode.toLowerCase()) {
            case "welcome":
                mainArea.currentIndex = 0;
                break;
            case "help":
                mainArea.currentIndex = 1;
                break;
            default:
                mainArea.currentIndex = 2;
                // TODO: update loader delegate for selected mode.
                break;
            }
        }
    }

    Loader {
        id: menuLoader
        Component.onCompleted: {
            const props = {
                "actions": actions,
                "project": Qt.binding(() => window.currentProject)
            };
            if (PlatformDetector.isWASM) {
                props["window"] = window;
                setSource("qrc:/qt/qml/edu/pepp/menu/QMLMainMenu.qml", props);
            } else
                // Auto-recurses on "parent" to find "window" of correct type.
                // If explicitly set, the menu bar will not render until hovered over.
                setSource("qrc:/qt/qml/edu/pepp/menu/NativeMainMenu.qml", props);
        }
        onLoaded: {
            if (PlatformDetector.isWASM)
                window.menuBar = item;
        }
        asynchronous: false
    }

    /*
     * Top-level dialogs
     */
    About.AboutDialog {
        id: aboutDialog
        parent: Overlay.overlay
        anchors.centerIn: parent

        // Do not create binding to settings directly, so that we don't get modified when the setting is updated.
        function getMin() {
            // By making a copy of the value before binding, we can avoid propogating updates to settings.
            var copy;
            if (whatsNewDialogSettings.lastOpenedVersion === "")
                copy = Version.version_str_full;
            else {
                copy = whatsNewDialogSettings.lastOpenedVersion;
            }

            //  Version string
            return copy;
        }

        //  Version settings
        Settings {
            id: whatsNewDialogSettings
            property string lastOpenedVersion
        }

        //  Clears settings based on developer menu option
        function onClearLastVersion() {
            whatsNewDialogSettings.lastOpenedVersion = "";
            whatsNewDialogSettings.sync();
        }

        //  Trigger About dialog if version has changed since last opening.
        Component.onCompleted: {
            actions.appdev.clearChangelogCache.triggered.connect(onClearLastVersion);
            if (whatsNewDialogSettings.lastOpenedVersion !== Version.version_str_full) {
                //  Version has changed. Open about to change log tab. Set old version.
                const version = getMin();
                aboutDialog.setMinimumVersion(version);
                aboutDialog.setTab(AboutDialog.TabType.ChangeLog);
                open();
            }
            whatsNewDialogSettings.lastOpenedVersion = Version.version_str_full;
        }
    }

    Dialog {
        id: preferencesDialog
        title: qsTr("Preferences")
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        height: 3 * 240 //Math.min(prefs.contentHeight + 100, 480)
        width: 3 * 320 //Math.min(prefs.contentWidth + 100, 640)
        contentItem: AppSettings.TopLevel {
            id: prefs
            focus: true
            anchors {
                margins: parent.padding
                left: parent.left
                right: parent.right
                top: parent.header.bottom
                bottom: parent.footer.top
            }
            Keys.onEscapePressed: {
                preferencesDialog.close();
            }
        }
        standardButtons: Dialog.Close
        onAccepted: prefs.closed()
        onClosed: prefs.closed()
    }

    Dialog {
        id: fileDisambiguateDialog
        title: qsTr("Determine file type")
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        height: parent.height
        width: parent.width
        contentItem: Top.Welcome {
            id: welcomeForFOpen
            focus: true
            Keys.onEscapePressed: {
                fileDisambiguateDialog.close();
            }
            onAddProject: function (arch, abs, feats, content, reuse) {
                const prj = window.pm.onAddProject(arch, abs, feats, content, reuse);
                const name = welcomeForFOpen.loadingFileName;
                const idx = window.pm.index(window.pm.currentProjectRow, 0);
                window.pm.setData(idx, name, window.pm.roleForName("path"));
                settings.general.pushRecentFile(name, arch, abs);
                sidebar.switchToMode("Editor");
                welcomeForFOpen.loadingFileName = Qt.binding(() => "");
                welcomeForFOpen.loadingFileContent = Qt.binding(() => "");
                welcomeForFOpen.filterAbstraction = Qt.binding(() => []);
                welcomeForFOpen.filterEdition = Qt.binding(() => []);
                fileDisambiguateDialog.accept();
            }
        }
        standardButtons: Dialog.Close
    }

    FileIO {
        id: fileio
        onCodeLoaded: function (name, content, arch, abs) {
            if (!name || !content)
                return;
            if (arch !== 0 && abs !== 0) {
                const prj = window.pm.onAddProject(arch, abs, "", content, true);
                const idx = window.pm.index(window.pm.currentProjectRow, 0);
                window.pm.setData(idx, name, window.pm.roleForName("path"));
                settings.general.pushRecentFile(name, arch, abs);
                sidebar.switchToMode("Editor");
                return;
            } else if (name.match(/pep$/i)) {
                welcomeForFOpen.filterAbstraction = Qt.binding(() => [Abstraction.ASMB3, Abstraction.OS4, Abstraction.ASMB5]);
                welcomeForFOpen.filterEdition = Qt.binding(() => [6, 5, 4]);
            } else if (name.match(/pepo$/i)) {
                welcomeForFOpen.filterAbstraction = Qt.binding(() => [Abstraction.ISA3]);
                welcomeForFOpen.filterEdition = Qt.binding(() => [6, 5, 4]);
            } else if (name.match(/pepcpu$/i)) {
                welcomeForFOpen.filterAbstraction = Qt.binding(() => [Abstraction.MC2]);
                welcomeForFOpen.filterEdition = Qt.binding(() => [6, 5, 4]);
            } else if (name.match(/pepm$/i)) {
                welcomeForFOpen.filterAbstraction = Qt.binding(() => [Abstraction.ASMB3, Abstraction.OS4, Abstraction.ASMB5]);
                welcomeForFOpen.filterEdition = Qt.binding(() => [6]);
            } else {
                welcomeForFOpen.filterAbstraction = Qt.binding(() => []);
                welcomeForFOpen.filterEdition = Qt.binding(() => []);
            }

            sidebar.switchToMode("Welcome");
            welcomeForFOpen.loadingFileName = Qt.binding(() => name);
            welcomeForFOpen.loadingFileContent = Qt.binding(() => content);
            fileDisambiguateDialog.open();
        }
    }

    function onNew() {
        pm.onAddProject(settings.general.defaultArch, settings.general.defaultAbstraction, "", "", true);
        sidebar.switchToMode("Editor");
    }
    function onOpenDialog() {
        fileio.loadCodeViaDialog("");
    }
    // must be named onOpenFile, or `gui.cpp` must be updated!
    function onOpenFile(filename, arch, abs) {
        fileio.loadCodeFromFile(filename, arch, abs);
    }
    function onSaveAs(extension) {
        pm.onSaveAs(currentProjectRow, extension);
    }
    // You (the caller) must set inRecurseClose if you call recurseClose.
    // You will be notified that iteration/recursion stopped via onRecurseCloseFinished.
    // When you receive that signal, you must set inRecurseClose back to false.
    // We have to used chained events rather than iterations since dialogs are asynchronous,
    // and closing projects may spawn dialogs.
    property bool inRecurseClose: false
    signal onRecurseCloseFinished
    function recurseClose(eat_arg) {
        if (pm.rowCount() == 0)
            onRecurseCloseFinished();
        // Must defer execution until re-entering the event loop, or dialogs may be lost.
        Qt.callLater(() => projectSelect.closeProject(0, true));
    }

    function onCloseAllProjects(excludeCurrent: bool) {
        // Cleanup all the connections we made, otherwise adding another project will cause all projects to close
        // due to rowCountChanged.
        const cleanup = () => {
            pm.rowCountChanged.disconnect(recurseClose);
            onRecurseCloseFinished.disconnect(cleanup);
            inRecurseClose = Qt.binding(() => false);
        };
        // Recursively call recurseClose every time the row count changes.
        // Again, close is async, so we have to use this chaining style.
        pm.rowCountChanged.connect(recurseClose);
        inRecurseClose = Qt.binding(() => true);
        // When recursion terminates, perform previously listed cleanup.
        onRecurseCloseFinished.connect(cleanup);
        // Begin recursion
        recurseClose();
    }

    function onQuit() {
        // Let users escape saving hell by quiting a second time.
        if (inRecurseClose)
            Qt.quit();
        else {
            onRecurseCloseFinished.connect(() => Qt.quit());
            onCloseAllProjects(false);
        }
    }
    function onToggleFullScreen() {
    }
}
