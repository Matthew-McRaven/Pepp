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
import "qrc:/qt/qml/Pepp/gui/about" as About
import "qrc:/qt/qml/Pepp/gui/helpview" as Help
import "qrc:/ui/memory/hexdump" as Memory
import "qrc:/qt/qml/Pepp/gui/cpu" as Cpu
import "qrc:/qt/qml/Pepp/gui/project"

ApplicationWindow {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Pep/10 Help")
    ButtonGroup {
        buttons: sidebar.children
    }

    property string mode: "MEMDEMO"
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

            MenuSeparator {
            }
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

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: qsTr("‹")
            }
            Label {
                text: "Title"
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
            ToolButton {
                text: qsTr("⋮")
                onClicked: menu.open()
            }
        }
    }

    Column {
        id: sidebar
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 100
        SideButton {
            text: "memdemo"; checked: true; onClicked: window.mode = "MEMDEMO"
        }
        SideButton {
            text: "welcome"; onClicked: window.mode = "WELCOME"
        }
        SideButton {
            text: "edit"; onClicked: window.mode = "EDIT"
        }
        SideButton {
            text: "debug"; onClicked: window.mode = "DEBUG"
        }
        SideButton {
            text: "help"; onClicked: window.mode = "HELP"
        }
    }
    StackLayout {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: sidebar.right
        width: parent.width
        currentIndex: window.mode === "HELP" ? 2 : (window.mode === "WELCOME" ? 0 : (window.mode === "MEMDEMO" ? 3 : 1))

        Rectangle {
            color: "Orange"
            Text {
                text: "Welcome"
                anchors.centerIn: parent
            }
        }
        Item {
            TabBar {
                id: projectSelect
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.top: parent.top
                height: 30
                TabButton {
                    text: qsTr("Fig 1")
                    height: parent.height - 4
                }
                TabButton {
                    text: qsTr("Fig 2")
                    height: parent.height - 4
                }
                TabButton {
                    text: qsTr("Fig 3")
                    height: parent.height - 4
                }
            }
            StackLayout {
                anchors.top: projectSelect.bottom; anchors.bottom: parent.bottom
                anchors.left: parent.left; anchors.right: parent.right
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
            }

            StackLayout {
                currentIndex: tab.currentIndex
                width: wrapper.width
                //Layout.top: tab.bottom
                //Layout.bottom: wrapper.bottom
                anchors.top: tab.bottom
                anchors.bottom: wrapper.bottom

                Memory.MemoryDump {
                }
                Cpu.Cpu {
                }
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
