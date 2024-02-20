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
//  Figure contents
ScrollView {
    id: wrapper
    Layout.alignment: Qt.AlignCenter
    Layout.fillHeight: true;
    Layout.fillWidth: true
    required property bool isReadOnly;
    required property string edition;
    required property string language;
    property string text;


    //  Set page contents based on parent selected values
    Component.onCompleted: {
        DefaultStyles.pep10_asm(styles)
        DefaultStyles.c(styles)
        highlighter.set_styles(styles)
        highlighter.set_document(figContent.textDocument)
        highlighter.set_highlighter(edition, language)
    }

    StyleMap {
        id: styles
    }
    BlockFinder {
        id: finder
    }
    Highlighter {
        id: highlighter
    }

    LineNumbers {
        id: lineNumbers
        height: parent.height // Ensure that line numbering area spans entire text area.
        width: 40
    }
    TextArea {
        id: figContent
        textFormat: TextEdit.PlainText
        renderType: Text.NativeRendering

        function update() {
            lineNumbers.lineCount = lineCount
            // font metrics lies about height, because it does not include intra-line padding.
            // instead, math out from content height the line height.
            // lineNumbers.lineHeight = metrics.height
            lineNumbers.lineHeight = contentHeight / lineCount
            // Use my C++ helper code to determine line number from cursors integer.
            lineNumbers.cursorPosition = finder.find_pos(cursorPosition)
            lineNumbers.selectionStart = finder.find_pos(selectionStart)
            lineNumbers.selectionEnd = finder.find_pos(selectionEnd)
            lineNumbers.update()  // Graphics area will never update without requesting it.
        }

        // Anchor otherwise line numbers overlap text edit.
        anchors.left: lineNumbers.right

        onLineCountChanged: update()
        onHeightChanged: update()
        onCursorPositionChanged: update()

        onSelectedTextChanged: update()

        font.family: "Courier New"

        //textFormat: TextEdit.RichText
        //wrapMode: TextEdit.NoWordWrap
        readOnly: wrapper.isReadOnly;

        text: wrapper.text
    }
}
