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
import QtQuick.Controls.Material

Item {
    //  Set by parent - formatting
    property alias hex16: bin.hex16
    property alias showPrefix: bin.showPrefix

    //  Data elements
    property alias register: label.text
    property alias address: bin.value
    property alias value: value2.text

    property int cellWidth: 60
    //  For testing only. Overridden by parent
    //anchors.fill: parent
    width: 500
    height: 20

    RowLayout {
        id: wrapper
        anchors.fill: parent
        spacing: 5

        Label {
            id: label

            Layout.fillHeight: true
            Layout.fillWidth: true

            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight

            text: "Register"
        }

        Rectangle {
            id: display1

            implicitHeight: childrenRect.height ? childrenRect.height : 20
            implicitWidth: cellWidth

            Layout.maximumWidth: cellWidth
            Layout.minimumWidth: cellWidth

            anchors.margins: 1
            Rectangle {
                border.color: Material.foreground
                color: Material.background
                border.width: 1
                anchors.fill: parent
                Label {
                    id: bin
                    property int value: address
                    property bool showPrefix: false
                    property bool hex16: true

                    anchors.fill: parent
                    text: qsTr("%1%2")
                        .arg(showPrefix ? "0b" : "")
                        .arg((value % (hex16 ? 0xffff : 0xff)).toString(2).padStart(hex16 ? 16 : 8, '0'))
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        Rectangle {
            id: display2

            implicitHeight: childrenRect.height ? childrenRect.height : 20
            implicitWidth: cellWidth

            Layout.maximumWidth: cellWidth
            Layout.minimumWidth: cellWidth

            anchors.margins: 1

            //border.color: Material.foreground
            color: Material.background
            //border.width: 1

            Label {
                id: value2

                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                padding: 0
                color: Material.foreground
                text: "0"
            }
        }
    }
}
