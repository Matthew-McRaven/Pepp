

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
import Qt.labs.qmlmodels
import "../cpu" as Ui

Rectangle {
    id: wrapper
    property alias registers: registers.model
    property alias flags: flags.model
    FontMetrics {
        id: metrics
    }
    RowLayout {
        id: flagsContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: childrenRect.height
        Repeater {
            id: flags
            delegate: Column {
                required property string display
                required property bool value
                Layout.fillWidth: true
                Layout.fillHeight: true
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: display
                }
                CheckBox {
                    anchors.horizontalCenter: parent.horizontalCenter
                    enabled: false
                    checked: value
                }
            }
        }
    }

    // TODO: switch to Row+Repeater
    TableView {
        id: registers
        anchors.top: flagsContainer.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: contentHeight

        columnWidthProvider: function (column) {
            return (registers.model.columnCharWidth(
                        column) + 2) * metrics.averageCharacterWidth
        }
        delegate: Component {
            Text {
                required property bool readOnly
                required property string display
                text: display
                font: metrics.font
            }
        }
    }
}
