
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
import edu.pepp 1.0 as Pepp

ScrollView {
    id: wrapper
    property int bytesPerRow: 16
    property alias readOnly: editor.readOnly
    property alias text: editor.text
    signal editingFinished(string text)
    NuAppSettings {
        id: settings
    }

    Component.onCompleted: {
        // Propogate editingFinished to containing component
        editor.editingFinished.connect(text => wrapper.editingFinished(text))
    }
    Rectangle {
        anchors.fill: parent
        color: palette.base
        //  Give object code viewer a background box
        border.width: 1
        border.color: palette.mid
    }
    Pepp.ObjectUtilities {
        id: utils
        bytesPerRow: wrapper.bytesPerRow
    }
    // If I put TextArea directly in parent, parent shrinks to the size of the TextArea
    // This happens even when I try to force fillHeight / fillWidth in the containing Layout.
    // Nesting the editor defeats this behavior.
    Item {
        anchors.fill: parent
        TextArea {
            id: editor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            textFormat: TextEdit.PlainText
            renderType: Text.NativeRendering
            font: settings.extPalette.baseMono.font
            readOnly: wrapper.isReadOnly
            Keys.onPressed: event => {
                                if (event.key === Qt.Key_Insert
                                    && event.modifiers === Qt.NoModifier)
                                editor.overwriteMode = !editor.overwriteMode
                                else
                                // If event is accepted, it won't reach the actual TextArea
                                // Use that behavior to filter out "wrong" keys.
                                event.accepted = !utils.valid(event.key)
                            }
        }
    }
}
