

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
import "qrc:/ui/about" as About
import "qrc:/ui/help" as Help
import "qrc:/ui/memory/hexdump" as Memory
import "qrc:/ui/cpu" as Cpu
import "qrc:/ui/text/editor" as Editor
import "qrc:/ui/project" as Project
import "qrc:/ui/preferences" as Pref
import edu.pepp 1.0
import "./menu" as Menu

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Pepp IDE")

    //  Set palette in parent. Inherited by all children
    /*  See https://doc.qt.io/qt-6/qml-qtquick-colorgroup.html for color explanation*/
    palette {
        alternateBase: Theme.container.background
        base: Theme.surface.background
        text: Theme.surface.foreground
        button: Theme.container.background
        buttonText: Theme.container.foreground
        highlight: Theme.primary.background
        highlightedText: Theme.primary.foreground
        window: Theme.container.background
        windowText: Theme.container.foregound

        //  Colors when control is disabled. Overrides normal palette
        disabled {
            highlight: Theme.container.background
        }
    }

    property var currentProject: null

    property string mode: "welcome"
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
        // console.log(`project: switching to ${index}`)
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
        console.log(`project: closed ${index}`)
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
        // Allow welcome mode to create a new project, and switch to it on creation.
        welcome.addProject.connect(pm.onAddProject)
        welcome.addProject.connect(() => switchToProject(pm.count - 1))
        welcome.addProject.connect(() => sidebar.switchToMode("edit"))
        help.addProject.connect(pm.onAddProject)
        help.addProject.connect(() => switchToProject(pm.count - 1))
        help.switchToMode.connect(sidebar.switchToMode)
        currentProjectChanged.connect(projectLoader.onCurrentProjectChanged)

        actions.edit.prefs.triggered.connect(preferencesDialog.open)
        actions.help.about.triggered.connect(aboutDialog.open)
        actions.view.fullscreen.triggered.connect(onToggleFullScreen)
        message.connect(text => footer.text = text)
        message.connect(() => messageTimer.restart())
        messageTimer.restart()
    }

    ProjectModel {
        id: pm
        function onAddProject(arch, level, feats, optTexts) {
            var proj = null
            // Attach a delegate to the project which can render its edit/debug modes. Since it is a C++ property,
            // binding changes propogate automatically.
            switch (Number(arch)) {
            case Architecture.PEP10:
                if (Number(level) === Abstraction.ISA3) {
                    proj = pm.pep10ISA(pep10isaComponent)
                } else {
                    proj = pm.pep10ASMB(pep10asmbComponent)
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
    }

    // Provide a default font for menu items.
    FontMetrics {
        id: menuFont
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

    menuBar: Menu.MainMenu {
        id: menu
        project: window.currentProject
        window: window
        actions: actions
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
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    action: actions.file.new_
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolSeparator {}
                ToolButton {
                    action: actions.build.execute
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.start
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolSeparator {}
                ToolButton {
                    action: actions.debug.continue_
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stop
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.step
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stepOver
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stepInto
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolButton {
                    action: actions.debug.stepOut
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: action.text.replace(/&/g, "")
                    text: ''
                }
                ToolSeparator {}
                Help.Converters {
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
                            text: index
                            font: menuFont.font
                            // TODO: Set to equal the width of the text + 2 spaces.
                            width: Math.max(100, projectSelect.width / 6)
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
                        // TODO: use "theme"'s hover color.
                        border.color: addProjectButton.hovered ? "blue" : "transparent"
                        border.width: 2
                        radius: 4
                    }
                    onClicked: {
                        // TODO: Use window.currentProject.env to create a new project with same features.
                        pm.onAddProject("pep/10", "isa3", "")
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
            // If there is no current project, display a Welcome mode.
            model: window.currentProject ? window.currentProject.modes(
                                               ) : defaultModel
            delegate: Project.SideButton {
                text: model.display ?? "ERROR"
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
        buttons: sidebar.children
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
        Help.HelpView {
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
        }
        Component.onCompleted: {
            window.modeChanged.connect(onModeChanged)
            onModeChanged()
        }
        function onModeChanged() {
            console.log(`Changed to ${window.mode}`)
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
        }
    }
    Component {
        id: pep10asmbComponent
        Project.Pep10ASMB {
            project: window.currentProject
            anchors.fill: parent
            mode: window.mode
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
        id: preferencesDialog
        title: qsTr("Preferences")
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: 700 // TODO: prevent binding loop on preferences size.
        height: 700

        contentItem: Pref.Preferences {
            id: prefs
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.header.bottom
            anchors.bottom: parent.footer.top
            model: PreferenceModel
        }
        standardButtons: Dialog.Close
    }
    function onNew() {
        pm.onAddProject(Architecture.PEP10, Abstraction.ISA3, "")
    }
    function onOpenDialog() {}
    function onCloseAllProjects(excludeCurrent: bool) {}
    function onQuit() {}
    function onToggleFullScreen() {}
}
