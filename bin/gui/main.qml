

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
import "qrc:/qt/qml/Pepp/gui/helpview" as Help
import "qrc:/ui/memory/hexdump" as Memory
import "qrc:/ui/cpu" as Cpu
import "qrc:/qt/qml/Pepp/gui/project"
import "qrc:/ui/text/editor" as Editor
import "qrc:/ui/project" as Project
import "qrc:/ui/preferences" as Pref
import edu.pepp 1.0

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Pepp IDE")

    property variant currentProject: null
    property string mode: "welcome"
    function switchToProject(index) {
        console.log(`Switching to ${index}`)
        projectSelect.currentIndex = index
    }
    Component.onCompleted: {
        // Allow welcome mode to create a new project, and switch to it on creation.
        welcome.addProject.connect(pm.onAddProject)
        welcome.addProject.connect(() => switchToProject(pm.rowCount() - 2))
    }

    ProjectModel {
        id: pm
        function onAddProject(arch, level, feats) {
            pm.pep10ISA()
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

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Menu {
                title: "nested"
                Action {
                    text: qsTr("&Save Object")
                }
                Action {
                    text: qsTr("Save Object &As...")
                }
            }

            Action {
                text: qsTr("&New...")
            }
            Action {
                text: qsTr("&Open...")
            }

            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
            }
        }
        Menu {
            title: qsTr("&Edit")
            Action {
                text: qsTr("Cu&t")
            }
            Action {
                text: qsTr("&Copy")
            }
            Action {
                text: qsTr("&Paste")
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About")
                onTriggered: aboutDialog.open()
            }
            Action {
                text: qsTr("Preferences")
                onTriggered: preferencesDialog.open()
            }
        }
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
        ToolBar {
            id: toolbar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    text: qsTr("‹")
                    font: menuFont.font
                }
                Label {
                    text: "Title"
                    font: menuFont.font
                    elide: Label.ElideRight
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }
                ToolButton {
                    text: qsTr("⋮")
                    font: menuFont.font
                }
                Rectangle {
                    color: "transparent"
                    Layout.fillWidth: true
                }
            }
        }
        TabBar {
            id: projectSelect
            visible: Qt.binding(() => pm.rowCount() > 0)
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.top: toolbar.bottom
            clip: true // Prevent tabs from overpainting spacer.
            onCurrentIndexChanged: {
                window.currentProject = pm.data(pm.index(currentIndex, 0),
                                                ProjectModel.ProjectRole)
            }
            DelegateChooser {
                id: tabDelegateChooser
                role: "Type"
                DelegateChoice {
                    roleValue: "project"
                    TabButton {
                        // required property bool isPlus
                        text: display
                        font: menuFont.font
                        // Force the tab bar to become flickable if it is too crowded.
                        width: Math.max(100, projectSelect.width / 6)
                    }
                }
                DelegateChoice {
                    roleValue: "add"
                    TabButton {
                        text: "+"
                        font: menuFont.font
                        // Force the tab bar to become flickable if it is too crowded.
                        width: Math.max(100, projectSelect.width / 6)
                        // current index is updated before TabButton.onClicked(). Since I want access to the current project,
                        // I am using a mouse area to intercept the click before the tab bar changes.
                        MouseArea {
                            anchors.fill: parent
                            Component.onCompleted: {
                                // TODO: Use window.currentProject.env to create a new project with same features.
                                clicked.connect(() => pm.onAddProject("pep/10",
                                                                      "isa3",
                                                                      ""))
                                clicked.connect(() => switchToProject(
                                                    pm.rowCount() - 2))
                            }
                        }
                    }
                }
            }
            Repeater {
                model: pm
                anchors.fill: parent
                delegate: tabDelegateChooser
            }
        }
    }

    Column {
        id: sidebar
        anchors.top: headerSpacer.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 100
        Repeater {
            // If there is no current project, display a Welcome mode.
            model: window.currentProject ? window.currentProject.modes(
                                               ) : defaultModel
            delegate: SideButton {
                text: model.display ?? "ERROR"
                Component.onCompleted: {
                    // Triggers window.modeChanged, which will propogate to all relevant components.
                    onClicked.connect(() => {
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
            property alias model: window.currentProject
        }
        Loader {
            id: projectLoader
            Layout.fillHeight: true
            Layout.fillWidth: true
            // TODO: Will need to switch to "source" with magic for passing mode & model.
            sourceComponent: window.projectComponent
            property string mode: window.mode
            property alias model: window.currentProject
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
            required property string mode
            mode: pep10isaComponent.mode
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


        /*contentItem: Pref.Preferences {
            id: prefs
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.header.bottom
            anchors.bottom: parent.footer.top
        }*/
        standardButtons: Dialog.Close
    }
}
