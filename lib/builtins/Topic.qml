

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

Item {
    Column {
        id: column
        spacing: 20
        anchors {
            margins: 20
            left: parent.left
            right: parent.right
        }

        //  Topic name
        Label {
            id: title
            font.pixelSize: 22
            width: parent.width
            color: palette.brightText
            elide: Label.ElideRight
            horizontalAlignment: Qt.AlignLeft
            text: qsTr("Writing Programs")
            background: Rectangle {
                color: "#ff7d33"
                radius: 5
            }
        }

        //  Topic contents
        Label {
            id: content
            width: parent.width
            wrapMode: Label.WordWrap
            text: qsTr("Pep/10 is a virtual machine for writing machine language and assemply language programs") //.arg(inPortrait ? qsTr("portrait") : qsTr("landscape"))
        }
    }
}
