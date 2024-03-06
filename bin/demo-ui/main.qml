/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import Qt.labs.platform as Platform

import "ui" as Ui
import "qrc:/ui/memory/hexdump" as Memory
import "qrc:/ui/preferences" as Pref

ApplicationWindow {
    id: window
    objectName: "window"
    width: 1200
    height: 800
    visible: true
    title: qsTr("Pep/10")

    //property var currentFont: undefined

    Component.onCompleted: {
    }

    menuBar: Ui.PepMenuBar
    {
        id: menuBar
        aboutDialog: aboutDialog
        //textEditor: Ui.TextEditor
    }

    header: ToolBar {
        id: toolbar
    }
    footer: TabBar {
        id: footer
    }

    Item {
        id: wrapper
        width: parent.width
        anchors.top: toolbar.bottom
        anchors.bottom: footer.top

        TabBar {
            id: tab
            TabButton {
                text: "Memory Dump"
                width: implicitWidth
            }
            TabButton {
                text: "Preferences"
                width: implicitWidth
            }
            TabButton {
                text: "CPU"
                width: implicitWidth
            }
            TabButton {
                text: "Editor"
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
            Pref.Preferences {
                //anchors.fill: parent
                //Layout.fillHeight: true
                //Layout.fillWidth: true
            }

            Ui.Cpu {
                //anchors.fill: parent
            }
            Ui.TextEditor {
                //anchors.fill: parent
            }
        }
    }

    Platform.FileDialog {
        id: saveAsDialog
        //  0 = pep dialog, 1 = object dialog, 2 = listing dialog
        property int type: 0

        fileMode: Platform.FileDialog.SaveFile
        title: {
            if (type === 1) {
                "Open Pep/10 Object File"
            } else if (type === 2) {
                "Open Pep/10 Listing File"
            } else {
                "Open Pep/10 Source File"
            }
        }
        nameFilters: {
            if (type === 1) {
                ["Pep/10 Object files (*.pepo)", "Text files (*.txt)"]
            } else if (type === 2) {
                ["Pep/10 Listing (*.pepl)"]
            } else {
                ["Pep/10 Source (*.pep)", "Text files (*.txt)"]
            }
        }
        defaultSuffix: {
            if (type === 1) {
                "pepo"
            } else if (type === 2) {
                "pepl"
            } else {
                "pep"
            }
        }

        //onAccepted: project.saveAs(file)
    }

    //  Open differnt Pep/10 file types
    Platform.FileDialog {
        id: openFileDialog

        selectedNameFilter.index: 0
        nameFilters: ["Pep/10 Files (*.pep, *.pepo, *.pepl)",
            "Pep/10 Source (*.pep)",
            "Pep/10 Object files (*.pepo)",
            "Pep/10 Listing (*.pepl)"]
        defaultSuffix: "*.pep"
        //onAccepted: loadProject(file)
        //folder: (project && project.loaded) ? project.dirUrl : ""
    }

    //  Does not work on windows platform
    Platform.FontDialog {
        id: fontDialog
        /*currentFont: currentFont === undefined ?
        //    Font {
                         weight
          //if( currentFont === undefined ){
            currentFont.font.family = "Courier"
            currentFont.Normal = true;
            currentFont.font.pointSize = 11
          }
        }*/
        onAccepted: {
            //window.currentFont = currentFont
            //    currentFont.family: window.currentFont
        }
    }

    Ui.AboutDialog {
        id: aboutDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
    }
}
