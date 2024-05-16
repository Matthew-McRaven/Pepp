

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
    TextMetrics {
        id: tm
        font: metrics.font
        text: '0'
    }

    RowLayout {
        id: flagsContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Repeater {
            id: flags
            delegate: Row {
                required property string display
                required property bool value
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: -5
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: display
                }
                CheckBox {
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: false
                    checked: value
                }
            }
        }
    }
    ListView {
        id: registers
        anchors.top: flagsContainer.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        spacing: 1
        contentWidth: parent.width
        delegate: RowLayout {
            id: rowDelegate
            required property int index
            width: registers.width
            Repeater {
                model: registers.model.columnCount()
                delegate: Rectangle {
                    id: columnDelegate
                    required property int index
                    property int row: rowDelegate.index
                    property int column: index
                    property var mindex: registers.model.index(row, column)
                    property string display: registers.model.data(mindex)
                    property bool box: registers.model.data(mindex,
                                                            registers.model.Box)
                    property bool rightJustify: registers.model.data(
                                                    mindex,
                                                    registers.model.RightJustify)
                    Layout.minimumWidth: Math.max(35, textField.width)
                    Layout.minimumHeight: textField.height
                    Layout.fillWidth: true
                    TextField {
                        id: textField
                        background: Rectangle {
                            color: "transparent"
                            border.color: "black"
                            border.width: box ? 1 : 0
                            radius: 2
                        }
                        font: metrics.font
                        readOnly: true
                        maximumLength: registers.model.columnCharWidth(column)
                        anchors.centerIn: columnDelegate
                        text: display
                        horizontalAlignment: rightJustify ? Qt.AlignRight : Qt.AlignHCenter
                        // '0' is a wide character, and tm contains a single '0' in the current font.
                        width: tm.width * (maximumLength + 3)
                    }
                }
            }
        }
    }
}
