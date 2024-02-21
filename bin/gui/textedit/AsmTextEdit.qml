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

import "qrc:/qt/qml/Pepp/gui/components" as Ui
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

    property int colWidth: 30
    property int rows: 16


    //  Set page contents based on parent selected values
    Component.onCompleted: {
        DefaultStyles.pep10_asm(styles)
        DefaultStyles.c(styles)
        highlighter.set_styles(styles)
        highlighter.set_document(editor.textDocument)
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

    RowLayout {
        spacing: 0
        anchors.fill: parent
        Layout.fillHeight: true
        Layout.fillWidth: true

        //  Line numbers
        Ui.RowNumbers {
            id: rows
            Layout.topMargin: editor.topPadding + editor.textMargin
            Layout.bottomMargin: editor.bottomPadding
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignLeft
            Layout.maximumWidth: rows.width
            Layout.minimumWidth: rows.width

            width: wrapper.colWidth
            colWidth: wrapper.colWidth
            rowHeight: editor.lineHeight

            rows: editor.lineCount
            rowfont: editor.font
        }


        TextArea {
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignLeft
            id: editor
            textFormat: TextEdit.PlainText
            renderType: Text.NativeRendering
            property real lineHeight: contentHeight / lineCount
            font.family: "Courier New"

            //textFormat: TextEdit.RichText
            //wrapMode: TextEdit.NoWordWrap
            readOnly: wrapper.isReadOnly;

            text: wrapper.text
        }
    }
}
