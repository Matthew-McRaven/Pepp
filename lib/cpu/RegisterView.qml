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
import QtQuick.Controls.impl
import QtQuick.Layouts
import Qt.labs.qmlmodels
import "../cpu" as Ui
import edu.pepp

Column {
    id: layout
    property alias registers: registers.model
    property alias flags: flags.model
    NuAppSettings {
        id: settings
    }

    FontMetrics {
        id: metrics
        font: settings.extPalette.baseMono.font
    }
    TextMetrics {
        id: tm
        font: metrics.font
        text: '0'
    }
    TextMetrics {
        id: pm
        font: settings.extPalette.base.font
        text: 'Instruction Specifier'
    }
    Menu {
        id: contextMenu
        MenuItem {
            text: "Test"
        }
    }
    Component {
        id: menuItemComponent

        MenuItem {}
    }
    Row {
        id: flagsContainer
        property real overrideLeftMargin: 0
        spacing: metrics.averageCharacterWidth * 1.5
        Item {
            width: flagsContainer.overrideLeftMargin
            height: 1
        }

        Repeater {
            id: flags
            clip: true
            delegate: Row {
                id: del
                required property bool checked
                required property string text
                Rectangle {
                    id: borderRect
                    implicitWidth: innerText.implicitWidth + 2 * border.width + 2 * innerText.anchors.margins
                    implicitHeight: innerText.implicitHeight + 2 * border.width + 2 * innerText.anchors.margins
                    Text {
                        id: innerText
                        anchors.fill: parent
                        anchors.margins: 3
                        text: del.checked ? "1" : "0"
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        font: settings.extPalette.baseMono.font
                    }
                    color: "transparent"
                    border {
                        color: palette.shadow
                        width: 1
                    }
                    radius: 2
                }
                // Wrap label in item as work-around for Label not expanding to match height of borderRect
                Item {
                    Layout.fillHeight: true
                    implicitWidth: label.implicitWidth
                    implicitHeight: innerText.implicitHeight + 2 * borderRect.border.width + 2 * innerText.anchors.margins
                    Label {
                        id: label
                        leftPadding: 2
                        text: del.text
                        anchors.centerIn: parent
                    }
                }

                Item {
                    implicitHeight: 1
                    implicitWidth: 8
                }
            }
        }
    }
    Repeater {
        id: registers
        clip: true
        property int innerSpacing: 5

        function updateFlagMargin() {
            // Need an offset of 3 to account for border width of 1 + radius 2, maybe?
            flagsContainer.overrideLeftMargin = Qt.binding(() => registers.x + pm.width * 1.2 - 3);
        }
        onWidthChanged: {
            updateFlagMargin();
        }
        onXChanged: {
            updateFlagMargin();
        }

        delegate: RowLayout {
            id: rowDelegate
            required property int index
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            spacing: registers.innerSpacing
            Repeater {
                model: TransposeProxyModel {
                    sourceModel: RowFilterModel {
                        sourceModel: registers.model
                        row: rowDelegate.index
                    }
                }

                delegate: Rectangle {
                    id: columnDelegate
                    required property int index
                    property int row: rowDelegate.index
                    property int column: index
                    required property string display
                    required property bool box
                    required property bool rightJustify
                    required property int selected
                    required property var choices
                    Layout.minimumWidth: textField.width
                    Layout.minimumHeight: textField.height + 1
                    Layout.preferredWidth: childrenRect.width
                    color: "transparent"
                    Component.onCompleted: {
                        if (column == 1 && row == 0)
                            registers.updateFlagMargin();
                    }

                    TextField {
                        id: textField
                        background: Rectangle {
                            color: "transparent"
                            border.color: palette.shadow
                            border.width: box ? 1 : 0
                            radius: 2
                        }
                        font: column == 0 ? pm.font : metrics.font
                        readOnly: true
                        // Minimum length == len(ADDSP,SFX)
                        maximumLength: 2 + Math.max(registers.model.columnCharWidth(column), 9)
                        anchors.centerIn: columnDelegate
                        text: columnDelegate.display
                        color: palette.windowText
                        horizontalAlignment: rightJustify ? Qt.AlignRight : Qt.AlignLeft
                        // 'W' is a wide character, and tm contains a single 'W' in the current font.
                        // All characters should be same width in mono font, but previous experience (#604) tell me this is a lie.
                        // Now adjusts for different fonts in each column.
                        width: (column == 0 ? pm.width * 1.2 : (metrics.averageCharacterWidth * maximumLength)) + (2)
                        onPressed: function (mouse) {
                            if (mouse.button === Qt.RightButton) {
                                while (contextMenu.count) {
                                    contextMenu.removeItem(contextMenu.itemAt(0));
                                }
                                if (!choices)
                                    return;
                                for (var i = 0; i < choices.length; i++) {
                                    var menuItem = menuItemComponent.createObject(contextMenu.contentItem, {
                                        "text": qsTr(choices[i]),
                                        "checkable": true,
                                        "checked": i === selected
                                    });
                                    // Stupid QML formatter keeps resetting i to var, and changes its scope.
                                    const idx = i;
                                    const mindex = registers.model.index(row, column);
                                    menuItem.onTriggered.connect(function () {
                                        registers.model.setData(mindex, idx, registers.model.Selected);
                                    });
                                    contextMenu.addItem(menuItem);
                                }
                                if (contextMenu.count > 0)
                                    contextMenu.popup(textField);
                            }
                        }
                    }
                }
            }
        }
    }
    // Provide some padding at the bottom
    Item {
        Layout.fillHeight: true
    }
}
