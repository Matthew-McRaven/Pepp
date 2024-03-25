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
import edu.pepp 1.0

TableView {
    required property bool isReadOnly
    id: wrapper
    anchors.fill: parent
    rowSpacing: 0
    columnSpacing: 0
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    focus: true
    selectionBehavior: TableView.SelectCells
    selectionMode: TableView.ContiguousSelection
    property int editRow: -1
    property int editColumn: -1
    reuseItems: false
    //editTriggers: TableView.SingleTapped | TableView.EditKeyPressed

    ScrollBar.horizontal: ScrollBar {
        policy: ScrollBar.AlwaysOff
    }
    ScrollBar.vertical: ScrollBar {
        id: vsc
        policy: ScrollBar.AlwaysOn
    }
    visibleArea.onHeightRatioChanged: {
    }

    model: ObjectCodeModel {
    }

    TextMetrics {
        id: fm
        font.family: "Courier New"
        text: "ZZ"
    }

    delegate: Rectangle {
        id: delegate
        required property string display;
        implicitHeight: fm.height * 2
        implicitWidth: fm.width * 2
        Text {
            id: text
            anchors.fill: parent
            font: fm.font
            text: display
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
        TableView.editDelegate: TextField {
            id: editor
            selectionColor: "red"
            anchors.fill: parent
            font: fm.font
            text: display
            maximumLength: 2
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            overwriteMode: true
            validator: RegularExpressionValidator {
                //Either 2 hex chars, loader sentinel ZZ, or up to 2 spaces
                regularExpression: /^([0-9a-fA-F]){1,2}|([zZ]{2})|([ \n]{0,2})$/
            }
            Component.onCompleted: {
                wrapper.editRow = row
                wrapper.editColumn = column
                focus = true
                editor.cursorPosition = 0
            }
            Component.onDestruction: {
                wrapper.editRow = wrapper.editColumn = -1
            }
            TableView.onCommit: {
                onEditingFinished()
            }
            onTextEdited: {
                if (editor.cursorPosition >= 2) {
                    onEditingFinished()
                }
            }
            onEditingFinished: {
                model.display = editor.text
                //const index = wrapper.model.index(wrapper.editRow, wrapper.editColumn)
                // wrapper.model.setData(index, editor.text, Qt.DisplayRole)
                let w = wrapper
                Qt.callLater(()=>w.keyPressed(Qt.Key_Right))
            }

            Keys.onPressed: (event) => {
                //  Key events that we track at TableView
                const key = event.key
                let isMoving = false
                switch (key) {
                    case Qt.Key_Left:
                        isMoving = editor.cursorPosition === 0
                        break
                    case Qt.Key_Right:
                        isMoving = editor.cursorPosition + 1 >= 2
                        break
                    case Qt.Key_Up:
                    // @disable-check M20
                    case Qt.Key_Down:
                        isMoving = true
                        break
                    default:
                        break
                }
                if (isMoving) {
                    Qt.callLater(onEditingFinished)
                } else {
                    event.accepted = false
                }
            }
        }
    }
    //  Capture movement keys in table view
    Keys.onPressed: (event) => {
        event.accepted = keyPressed(event.key)
    }

    function keyPressed(key) {
        let next = null
        switch (key) {
            case Qt.Key_Left:
                next = model.left(editRow, editColumn)
                break
            case Qt.Key_Right:
                next = model.right(editRow, editColumn)
                break
            case Qt.Key_Up:
                next = model.up(editRow, editColumn)
                break
            case Qt.Key_Down:
                next = model.down(editRow, editColumn)
                break
            default:
                return false
        }
        edit(next)
        // Transfer focus to sibling of editor to eliminate "sticky" focus
        return true
    }
}
