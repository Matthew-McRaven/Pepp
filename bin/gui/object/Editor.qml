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
        id: model
    }
    TextMetrics {
        id: fm
        font.family: "Courier New"
        text: "ZZ"
    }

    delegate: Rectangle {
        id: delegate
        required property var display;
        implicitHeight: fm.height * 2
        implicitWidth: fm.width * 2
        Text {
            id: text
            anchors.fill: parent
            font: fm.font
            text: delegate.display ?? ""
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
        TableView.editDelegate: TextField {
            anchors.fill: parent
            text: display
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            Component.onCompleted: selectAll()
            TableView.onCommit: {
                display = text
                // 'display = text' is short-hand for:
                // let index = TableView.view.index(row, column)
                // TableView.view.model.setData(index, text, Qt.DisplayRole)
            }
        }
    }
}
