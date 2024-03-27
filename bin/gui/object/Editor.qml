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
        width: 100
        height: 35
        TextField {
            text: display
            anchors.fill: parent
            onEditingFinished: model.display = text
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
        // Transfer focus to sibling of editor to eliminate "sticky" focus
        return true
    }
}
