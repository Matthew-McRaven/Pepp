

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
import "qrc:/edu/pepp/components" as Comp
import "qrc:/edu/pepp/memory/hexdump" as Memory
import "qrc:/edu/pepp/cpu" as Cpu
import "qrc:/edu/pepp/text/editor" as Editor
import "qrc:/edu/pepp/project" as Project
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
            buttonText: settings.extPalette.shadow.foreground
            button: settings.extPalette.base.background
        }
    }

    property var currentProject: null
    // Used to expose actions to inner area.
    property var actionRef: actions
    property string mode: "welcome"
    function setProjectCharIn(charIn) {
        if (currentProject)
            currentProject.charIn = charIn
    }
    function setCurrentProject(index) {
        if (window?.currentProject?.message !== undefined) {
            window.currentProject.message.disconnect(window.message)
        }
        window.currentProject = pm.data(pm.index(index, 0),
                                        ProjectModel.ProjectRole)
        if (window.currentProject.message)
            window.currentProject.message.connect(window.message)
    }

    function switchToProject(index, force) {
        var needsManualUpdate = (force ?? false)
                && projectBar.currentIndex === index
        if (needsManualUpdate)
            setCurrentProject(index)
        else
            projectBar.currentIndex = index
    }

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

    function closeProject(index) {
        // TODO: add logic to save project before closing or reject change entirely.
        pm.removeRows(index, 1)
        if (pm.rowCount() === 0)
            return
        else if (index < pm.rowCount())
            switchToProject(index, true)
        else
            switchToProject(pm.rowCount() - 1)
    }

    Component.onCompleted: {
        //console.log(ApplicationPreferences)
        // Allow welcome mode to create a new project, and switch to it on creation.
        welcome.addProject.connect(pm.onAddProject)
        welcome.addProject.connect(() => switchToProject(pm.count - 1))
        welcome.addProject.connect(() => sidebar.switchToMode("Editor"))
        help.addProject.connect(pm.onAddProject)
        help.addProject.connect(() => switchToProject(pm.count - 1))
        help.switchToMode.connect(sidebar.switchToMode)
        help.setCharIn.connect(i => setProjectCharIn(i))
        currentProjectChanged.connect(projectLoader.onCurrentProjectChanged)

        actions.edit.prefs.triggered.connect(preferencesDialog.open)
        actions.help.about.triggered.connect(aboutDialog.open)
        actions.view.fullscreen.triggered.connect(onToggleFullScreen)
        message.connect(text => footer.text = text)
        message.connect(() => messageTimer.restart())
        messageTimer.restart()
        sidebar.switchToMode("Welcome")
    }

    ProjectModel {
        id: pm
        function onAddProject(arch, level, feats, optTexts, reuse) {
            var proj = null
            var cur = window.currentProject
            // Attach a delegate to the project which can render its edit/debug modes. Since it is a C++ property,
            // binding changes propogate automatically.
            switch (Number(arch)) {
            case Architecture.PEP10:
                if (Number(level) === Abstraction.ISA3) {
                    if (cur && cur.architecture === Architecture.PEP10
                            && cur.abstraction === Abstraction.ISA3
                            && cur.isEmpty && reuse)
                        proj = cur
                    else
                        proj = pm.pep10ISA(pep10isaComponent)
                } else if (Number(level) === Abstraction.ASMB3) {
                    if (cur && cur.architecture === Architecture.PEP10
                            && cur.abstraction === Abstraction.ASMB3
                            && cur.isEmpty && reuse)
                        proj = cur
                    else
                        proj = pm.pep10ASMB(pep10asmbComponent,
                                            Abstraction.ASMB3)
                } else if (Number(level) === Abstraction.ASMB5) {
                    if (cur && cur.architecture === Architecture.PEP10
                            && cur.abstraction === Abstraction.ASMB5
                            && cur.isEmpty && reuse)
                        proj = cur
                    else
                        proj = pm.pep10ASMB(pep10asmbComponent,
                                            Abstraction.ASMB5)
                }
                break
            case Architecture.PEP9:
                if (Number(level) === Abstraction.ISA3) {
                    if (cur && cur.architecture === Architecture.PEP9
                            && cur.abstraction === Abstraction.ISA3
                            && cur.isEmpty && reuse)
                        proj = cur
                    else
                        proj = pm.pep9ISA(pep9isaComponent)
                } else if (Number(level) === Abstraction.ASMB5) {
                    if (cur && cur.architecture === Architecture.PEP9
                            && cur.abstraction === Abstraction.ASMB5
                            && cur.isEmpty && reuse)
                        proj = cur
                    else
                        proj = pm.pep9ASMB(pep9asmbComponent)
                }
                break
            }
            if (optTexts === undefined || optTexts === null)
                return
            else if (typeof (optTexts) === "string")
                proj.set(level, optTexts)
            else {
                for (const list of Object.entries(optTexts))
                    proj.set(list[0], list[1])
            }
        }
    }
    ListModel {
        id: defaultModel
        ListElement {
            display: "Welcome"
        }
        ListElement {
            display: "Help"
        }
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
        anchors.left: headerSpacer.right
        text: "test message"
        Timer {
            id: messageTimer
            interval: 10000
            onTriggered: window.footer.text = ""
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

    Item {
        // Intersection of header and mode select.
        // Make transparent, influenced by Qt Creator Style.
        id: headerSpacer
        anchors.top: parent.top
        anchors.left: parent.left
        width: sidebar.width
        height: header.height
    }
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: headerSpacer.right
        anchors.right: parent.right
        // Must explicitly set height to avoid binding loop; only account for tab bar if visibile.
        height: toolbar.height + (projectSelect.visible ? projectSelect.height : 0)
        // TODO: fix disabled icon colors
        ToolBar {
            id: toolbar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            property int iconHeight: 30
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    action: actions.file.new_
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolSeparator {}
                ToolButton {
                    action: actions.build.execute
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.start
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolSeparator {}
                ToolButton {
                    action: actions.debug.continue_
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stop
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.step
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stepOver
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stepInto
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stepOut
                    icon {
                        source: action.icon.source
                        height: toolbar.iconHeight
                        width: toolbar.iconHeight
                    }
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolSeparator {}
                Comp.Converters {
                    initialValue: 'a'.charCodeAt(0)
                    mnemonics: currentProject?.mnemonics ?? null
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        Flickable {
            id: projectSelect
            clip: true
            visible: pm.count > 0
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.top: toolbar.bottom
            contentWidth: projectBar.width + addProjectButton.width
            contentHeight: projectBar.height
            height: contentHeight
            // Use a row layout to handle proper sizing of tab bar and buttons.
            Row {
                TabBar {
                    id: projectBar
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onCurrentIndexChanged: window.setCurrentProject(
                                               currentIndex)
                    Repeater {
                        model: pm
                        anchors.fill: parent
                        delegate: TabButton {
                            // required property bool isPlus
                            text: `${index}<br>${pm.describe(index)}`
                            font: menuFont.font
                            width: Math.max(200, projectSelect.width / 4,
                                            implicitContentWidth)
                            Button {
                                text: "X"
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                height: Math.max(20, parent.height - 2)
                                width: height
                                onClicked: window.closeProject(index)
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
                        pm.onAddProject(window.currentProject.architecture,
                                        window.currentProject.abstraction, "")
                        window.switchToProject(pm.count - 1)
                    }
                }
            }
        }
    }

    Column {
        id: sidebar
        anchors.top: headerSpacer.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 100
        function switchToMode(mode) {
            // Match the button, case insensitive.
            const re = new RegExp(mode, "i")
            // Children of sidebar are the repeater's delegates
            for (var button of sidebar.children) {
                if (re.test(button.text)) {
                    button.clicked()
                    // Must set checked in order for ButtonGroup to match current mode.
                    button.checked = true
                    return
                }
            }
            console.error(`Did not find mode ${mode}`)
        }

        Repeater {
            id: sidebarRepeater
            // If there is no current project, display a Welcome mode.
            model: window.currentProject ? window.currentProject.modes(
                                               ) : defaultModel
            delegate: Button {
                checkable: true
                width: 100
                height: 65
                text: model.display ?? "ERROR"
                ButtonGroup.group: modeGroup
                Component.onCompleted: {
                    // Triggers window.modeChanged, which will propogate to all relevant components.
                    onClicked.connect(function () {
                        window.mode = text.toLowerCase()
                    })
                }
            }
        }
    }
    // Make sidebar buttons mutually-exclusive.
    ButtonGroup {
        id: modeGroup
    }

    StackLayout {
        id: mainArea
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: sidebar.right
        Project.Welcome {
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
        pm.onAddProject(Architecture.PEP10, Abstraction.ISA3, "", false)
    }
    function onOpenDialog() {}
    function onCloseAllProjects(excludeCurrent: bool) {}
    function onQuit() {
        window.close()
    }
    function onToggleFullScreen() {}
}
