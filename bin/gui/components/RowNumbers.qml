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
import QtQuick.Controls.Material

Rectangle {
    id: root

    //  For testing only. Overridden by parent
    width: 1000
    height: 800
    color: "#e0e0e0"

    //  Properties overriden by parent. Set defaults for testing
    property real colWidth: 30
    property real rowHeight: 20
    property real bulletSize: rowHeight * .8
    property int rows: 16
    required property font rowfont;
    property alias backgroundColor: root.color
    property color textColor: "#888888"
    property color highlightColor: "#f8f8f8"
    property bool enableBreakpoints: true

    property alias currentRow: view.currentIndex

    ListView {
        id: view
        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom;
        width: colWidth;

        clip: false //true
        focus: false
        interactive: false

        model: rows
        delegate:
            Rectangle {
                id: row
                color: view.currentIndex === index ? root.highlightColor : root.backgroundColor
                width: view ? ListView.view.width : 0
                height: root.rowHeight
                required property int index
                Rectangle {
                    id: bullet
                    visible: false

                    //anchors.left: view.left
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    color: "red"
                    height: root.bulletSize
                    width: root.bulletSize
                    radius: root.bulletSize / 2
                }

                MouseArea {
                    visible: root.enableBreakpoints
                    anchors.fill: parent
                    onClicked: {
                        bullet.visible = !bullet.visible;
                        //console.info( wrapper.height, wrapper.width);
                    }
                }
                Label {
                    id: rowNum
                    anchors.left: bullet.right; anchors.right: parent.right
                    anchors.top: parent.top; anchors.bottom: parent.bottom
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 5; rightPadding: 5
                    font.bold: view.currentIndex === row.index
                    color: root.textColor
                    text: row.index + 1
                }
            }
    }
}
