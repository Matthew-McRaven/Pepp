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
    property int editRow: -1
    property int editColumn: -1
    id: wrapper
    anchors.fill: parent
    rowSpacing: 0
    columnSpacing: 0
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    focus: true
    selectionBehavior: TableView.SelectCells
    selectionMode: TableView.ContiguousSelection
    editTriggers: TableView.SingleTapped | TableView.EditKeyPressed | TableView.DoubleTapped

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
        required property string display
        // editDelegate onCommit can't access model.row/column, so store them here.
        required property int row
        required property int column
        implicitWidth: fm.width * 2
        implicitHeight: fm.height * 2
        KeyEmitter{
            id: keys
        }

        Text {
            id: display
            font: fm.font
            anchors.fill: parent
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            text: delegate.display

        }
        TableView.editDelegate:
            TextField {
            id: editor
            anchors.fill: parent
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter

            font: fm.font
            maximumLength: 2
            text: delegate.display
            overwriteMode: true
            Component.onCompleted: {
                console.log("i,r,c"+index+","+row+","+column)
                editor.cursorPosition = 0
            }

            onTextEdited: {
                if (editor.cursorPosition >= 2) {
                    Qt.callLater(keys.emitEnter)
                }
            }
            Component.onDestruction: {
                // Must emit right after enter has been processed, otherwise editor is deleted before commit.
                // Can't directly call Keys.onPressed, or editDelegate keeps old value on row insert.
                Qt.callLater(keys.emitRight)
                wrapper.forceActiveFocus()
            }

            TableView.onCommit: {
                display = text
                wrapper.editRow = delegate.row
                wrapper.editColumn = delegate.column
            }
            validator: RegularExpressionValidator {
                //Either 2 hex chars, loader sentinel ZZ, or up to 2 spaces
                regularExpression: /^([0-9a-fA-F]){1,2}|([zZ]{2})|([ \n]{0,2})$/
            }
            Keys.onPressed:(event)=>{
                const key = event.key
                let moving=false
                switch(key){
                    case Qt.Key_Up: // Fallthrough
                    case Qt.Key_Down: moving=true; break
                    case Qt.Key_Left: moving=(editor.cursorPosition===0);break
                    case Qt.Key_Right: moving=(editor.cursorPosition+1 >=2);break
                    default: break
                }
                if(moving) Qt.callLater(keys.emitEnter)
                else event.accepted=false
            }
        }
    }
    Keys.onPressed:(event)=> {
        const key = event.key
        console.log("Wrapper key press")
        if(wrapper.editRow === -1 || wrapper.editColumn === -1) return
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
        }
        if(next) {
            closeEditor()
            edit(next)
            editRow = editColumn = -1
        }
    }
}
