

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
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels
import QtCore
import "qrc:/edu/pepp/about" as About
import "qrc:/edu/pepp/memory/hexdump" as Memory
import "qrc:/edu/pepp/cpu" as Cpu
import "qrc:/edu/pepp/text/editor" as Editor
import "qrc:/edu/pepp/project" as Project
import "qrc:/edu/pepp/top" as Top
import "qrc:/edu/pepp/settings" as AppSettings
import "qrc:/edu/pepp/builtins" as Builtins
import edu.pepp 1.0
import "qrc:/edu/pepp/menu" as Menu
//  Native menu for apple, linux, and windows
import Qt.labs.platform as Labs

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
            currentProject.charIn = charIn
    }

    Component.onCompleted: {
        // Allow welcome mode to create a new project, and switch to it on creation.
        welcome.addProject.connect(pm.onAddProject)
        welcome.addProject.connect(() => projectSelect.switchToProject(
                                       pm.count - 1))
        welcome.addProject.connect(() => sidebar.switchToMode("Editor"))
        help.addProject.connect(pm.onAddProject)
        help.addProject.connect(() => projectSelect.switchToProject(
                                    pm.count - 1))
        help.switchToMode.connect(sidebar.switchToMode)
        help.setCharIn.connect(i => setProjectCharIn(i))
        help.renameCurrentProject.connect(pm.renameCurrentProject)
        currentProjectChanged.connect(projectLoader.onCurrentProjectChanged)

        actions.edit.prefs.triggered.connect(preferencesDialog.open)
        actions.help.about.triggered.connect(aboutDialog.open)
        actions.view.fullscreen.triggered.connect(onToggleFullScreen)
        actions.file.save.triggered.connect(() => {
                                                preAssemble()
                                                pm.onSave(currentProjectRow)
                                            })
        actions.appdev.reloadFigures.triggered.connect(
                    help.reloadFiguresRequested)
        projectSelect.message.connect(message)
        message.connect(text => footer.text = text)
        message.connect(() => messageTimer.restart())
        messageTimer.restart()
        sidebar.switchToMode("Welcome")
    }

    // Provide a default font for menu items.
    FontMetrics {
        id: menuFont
    }
    function syncEditors() {
        if (projectLoader.item)
            projectLoader.item.syncEditors()
    }

    // Helper to propogate to current delegate.
    function preAssemble() {
        if (projectLoader.item)
            projectLoader.item.preAssemble()
    }

    Menu.Actions {
        id: actions
        project: window.currentProject
        window: window
    }

    signal message(string message)
    footer: Label {
        anchors.left: sidebar.right
        anchors.leftMargin: 10
        text: "test message"
        Timer {
            id: messageTimer
            interval: 10000
            onTriggered: window.footer.text = ""
        }
    }

    Top.ToolBar {
        id: toolbar
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
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: sidebar.right
        anchors.top: parent.top
        anchors.bottom: footer.bottom
        color: palette.shadow
    }

    Top.SideBar {
        id: sidebar
        modesModel: window.currentProject ? window.currentProject.modes(
                                                ) : undefined
        anchors {
            top: toolbar.bottom
            bottom: parent.bottom
            left: parent.left
        }
        width: 100

        onModeChanged: function (text) {
            window.mode = text.toLowerCase()
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
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        Builtins.HelpRoot {
            id: help
            Layout.fillHeight: true
            Layout.fillWidth: true
            abstraction: currentProject?.abstraction ?? Abstraction.NONE
            architecture: currentProject?.architecture ?? Architecture.NONE
        }
        Loader {
            id: projectLoader
            Layout.fillHeight: true
            Layout.fillWidth: true
            sourceComponent: null
            // Must unload the previous component to properly trigger save.
            function onCurrentProjectChanged() {
                sourceComponent = null
                sourceComponent = window.currentProject?.delegate
            }
            Connections {
                target: projectLoader.item
                function onRequestModeSwitchTo(mode) {
                    sidebar.switchToMode(mode)
                }
            }
        }
        Component.onCompleted: {
            window.modeChanged.connect(onModeChanged)
            onModeChanged()
        }
        function onModeChanged() {
            switch (window.mode.toLowerCase()) {
            case "welcome":
                mainArea.currentIndex = 0
                break
            case "help":
                mainArea.currentIndex = 1
                break
            default:
                mainArea.currentIndex = 2
                // TODO: update loader delegate for selected mode.
                break
            }
        }
    }

    // Helpers to render central component via Loader.
    Component {
        id: pep10isaComponent
        Project.Pep10ISA {
            project: window.currentProject
            anchors.fill: parent
            mode: window.mode
            actions: window.actionRef
        }
    }
    Component {
        id: pep9isaComponent
        Project.Pep10ISA {
            project: window.currentProject
            anchors.fill: parent
            mode: window.mode
            actions: window.actionRef
        }
    }
    Component {
        id: pep10asmbComponent
        Project.Pep10ASMB {
            project: window.currentProject
            anchors.fill: parent
            mode: window.mode
            actions: window.actionRef
        }
    }
    Component {
        id: pep9asmbComponent
        Project.Pep10ASMB {
            project: window.currentProject
            anchors.fill: parent
            mode: window.mode
            actions: window.actionRef
        }
    }

    Loader {
        id: menuLoader
        Component.onCompleted: {
            const props = {
                "actions": actions,
                "project": window.currentProject
            }
            if (PlatformDetector.isWASM) {
                props["window"] = window
                setSource("qrc:/edu/pepp/menu/QMLMainMenu.qml", props)
            } else
                // Auto-recurses on "parent" to find "window" of correct type.
                // If explicitly set, the menu bar will not render until hovered over.
                setSource("qrc:/edu/pepp/menu/NativeMainMenu.qml", props)
        }
        onLoaded: {
            if (PlatformDetector.isWASM)
                window.menuBar = item
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
    }
    Dialog {
        id: whatsNewDialog
        title: qsTr("What's New")
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: 700
        clip: true
        height: 700
        contentItem: Builtins.ChangelogViewer {
            // Do not create binding to settings directly, so that we don't get modified when the setting is updated.
            min: {
                // By making a copy of the value before binding, we can avoid propogating updates to settings.
                var copy
                if (whatsNewDialogSettings.lastOpenedVersion === "")
                    copy = Version.version_str_full
                else {
                    copy = whatsNewDialogSettings.lastOpenedVersion
                }
                // We still need to use a binding, or the model filtering will be unlinked from combo boxes.
                min = Qt.binding(() => copy)
            }
        }
        standardButtons: Dialog.Close
        Settings {
            id: whatsNewDialogSettings
            property string lastOpenedVersion
        }
        function onClearLastVersion() {
            whatsNewDialogSettings.lastOpenedVersion = ""
            whatsNewDialogSettings.sync()
        }
        Component.onCompleted: {
            actions.appdev.clearChangelogCache.triggered.connect(
                        onClearLastVersion)
            if (whatsNewDialogSettings.lastOpenedVersion !== Version.version_str_full)
                open()
            whatsNewDialogSettings.lastOpenedVersion = Version.version_str_full
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
            anchors {
                margins: parent.padding
                left: parent.left
                right: parent.right
                top: parent.header.bottom
                bottom: parent.footer.top
            }
        }
        standardButtons: Dialog.Close
        onAccepted: prefs.closed()
        onClosed: prefs.closed()
    }
    function onNew() {
        pm.onAddProject(Architecture.PEP9, Abstraction.ASMB5, "", false)
    }
    function onOpenDialog() {}
    function onCloseAllProjects(excludeCurrent: bool) {}
    function onQuit() {
        window.close()
    }
    function onToggleFullScreen() {}
}
