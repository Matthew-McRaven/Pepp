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

Item {
    id: project
    required property string mode
    property color color: "red"
    Rectangle {
        anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.left: parent.left; anchors.right: debugArea.visible ? debugArea.left : parent.right
        ComboBox {
            id: activeFile
            anchors.right: parent.right; anchors.left: parent.left
            anchors.top: parent.top; height: 40
            model: ["User", "OS"]
        }
        StackLayout {
            currentIndex: activeFile.currentIndex
            anchors.right: parent.right; anchors.left: parent.left
            anchors.top: activeFile.bottom; anchors.bottom: parent.bottom
            TextArea {
                text: "This is an user program"
                color: project.color
            }
            TextArea {
                text: "This is an operating system"
                color: project.color
            }
        }
    }
    Rectangle {
        id: debugArea
        visible: project.mode !== "EDIT"
        anchors.right: parent.right; width: 300
        anchors.top: parent.top; anchors.bottom: parent.bottom
        color: "lightgray"
    }

}
