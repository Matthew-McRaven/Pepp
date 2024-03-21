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

ScrollView {
    id: wrapper
    required property bool isReadOnly;
    property string text: "Hello cruel world";
    anchors.fill: parent
    Rectangle {
        anchors.fill: parent
        color: "white"
    }
    // If I put TextArea directly in parent, parent shrinks to the size of the TextArea
    // This happens even when I try to force fillHeight / fillWidth in the containing Layout.
    // Nesting the editor defeats this behavior.
    Item {
        anchors.fill: parent
        TextArea {
            id: editor
            anchors.left: parent.left; anchors.right: parent.right
            anchors.top: parent.top
            textFormat: TextEdit.PlainText
            renderType: Text.NativeRendering
            font.family: "Courier New"
            readOnly: wrapper.isReadOnly;
            text: wrapper.text
            Keys.onPressed: {
                return
            }
        }
    }
}
