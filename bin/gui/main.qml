

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

    property variant currentProject
    ProjectModel {
        id: pm
        function onAddProject(arch, level, feats) {}
    }
    // Provide a default font for menu items.
    FontMetrics {
        id: menuFont
    }
    // Helpers to render central component via Loader.
    Component {
        id: pep10isaComponent
        Project.Pep10ISA {
            required property string mode
            mode: pep10isaComponent.mode
        }
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
        height: toolbar.height + projectSelect.height
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
                    onClicked: menu.open()
                }
                Rectangle {
                    color: "transparent"
                    Layout.fillWidth: true
                }
            }
        }
        TabBar {
            id: projectSelect
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.top: toolbar.bottom
            onCurrentIndexChanged: {
                // TODO: handle OOB index, empty model.
                window.currentProject = Qt.binding(() => pm.get(currentIndex))
            }

            Repeater {
                model: pm
                anchors.fill: parent
                delegate: TabButton {
                    text: "Tab N"
                    font: menuFont.font
                    // Force the tab bar to become flickable if it is too crowded.
                    width: Math.max(100, projectSelect.width / 6)
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
        signal modeChanged(string mode)
        Repeater {
            model: window.currentProject || ["welcome"]
            delegate: SideButton {
                text: model.text ?? "ERROR"
                Component.onCompleted: {
                    onClicked.connect(() => sidebar.modeChanged(text))
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
            Layout.fillHeight: true
            Layout.fillWidth: true
            Component.onCompleted: {
                newProject.connect(pm.onAddProject)
            }
        }
        Help.HelpView {
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
            property string mode: 'welcome'
            property alias model: window.currentProject
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
}
