

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
import "qrc:/ui/preferences" as Pref
import edu.pepp 1.0

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Pep/10 Help")
    ProjectModel {
        id: pm
    }
    ButtonGroup {
        buttons: sidebar.children
    }
    // Provide a default font for menu items.
    FontMetrics {
        id: menuFont
    }
    property string mode: "MEMDEMO"

    onModeChanged: {
        stack.updateCurrentIndex()
        pm.pep10ISA()
        // console.log(test.objectCodeText)
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
        SideButton {
            text: "memdemo"
            checked: true
            onClicked: window.mode = "MEMDEMO"
        }
        SideButton {
            text: "welcome"
            onClicked: window.mode = "WELCOME"
        }
        SideButton {
            text: "edit"
            onClicked: window.mode = "EDIT"
        }
        SideButton {
            text: "debug"
            onClicked: window.mode = "DEBUG"
        }
        SideButton {
            text: "help"
            onClicked: window.mode = "HELP"
        }
        SideButton {
            text: "object"
            onClicked: window.mode = "OBJECT"
        }
    }

    StackLayout {
        id: stack
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: sidebar.right
        width: 400

        function updateCurrentIndex() {
            var modes = {
                "MEMDEMO": 3,
                "WELCOME": 0,
                "EDIT": 1,
                "DEBUG": 1,
                "HELP": 2,
                "OBJECT": 4
            }
            currentIndex = Qt.binding(() => modes[window.mode])
        }

        Component.onCompleted: updateCurrentIndex()

        Rectangle {
            color: "Orange"
            Text {
                text: "Welcome"
                anchors.centerIn: parent
            }
        }
        Item {
            StackLayout {
                anchors.fill: parent
                currentIndex: projectSelect.currentIndex
                Project {
                    mode: window.mode
                    color: "red"
                }
                Project {
                    mode: window.mode
                    color: "green"
                }
                Project {
                    mode: window.mode
                    color: "blue"
                }
            }
        }
        Item {
            width: parent.width
            Help.HelpView {
                anchors.fill: parent
            }
        }
        Item {
            id: wrapper
            width: parent.width
            Layout.fillHeight: true

            TabBar {
                id: tab
                TabButton {
                    text: "Memory Dump"
                    width: implicitWidth
                }
                TabButton {
                    text: "CPU"
                    width: implicitWidth
                }
                TabButton {
                    text: "Preferences"
                    width: implicitWidth
                }
            }

            StackLayout {
                currentIndex: tab.currentIndex
                width: wrapper.width
                //Layout.top: tab.bottom
                //Layout.bottom: wrapper.bottom
                anchors.top: tab.bottom
                anchors.bottom: wrapper.bottom

                Memory.MemoryDump {}
                Cpu.Cpu {}
                Pref.Preferences {}
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 400
            Editor.ObjTextEditor {
                readOnly: false
                anchors.fill: parent
            }
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
