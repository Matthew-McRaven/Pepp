

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
import "qrc:/qt/qml/edu/pepp/text/editor" as Editor

Item {
    id: wrapper
    required property string title
    required property var payload
    required property string lexerLang

    ColumnLayout {
        id: figureLayout
        spacing: 10
        anchors {
            topMargin: 0
            leftMargin: 20
            rightMargin: 20
            bottomMargin: 20
            fill: parent
        }
        Editor.ScintillaAsmEdit {
            id: textArea
            Layout.alignment: Qt.AlignCenter
            Layout.fillHeight: true
            Layout.fillWidth: true
            editorFont: editorFM.font
            language: wrapper.lexerLang
            text: wrapper.payload.text
            readOnly: false
            Component.onCompleted: textArea.readOnly = true

            Connections {
                target: wrapper.payload
                function onTextChanged() {
                    textArea.readOnly = false
                    textArea.text = wrapper.payload.text
                    textArea.readOnly = true
                }
            }
        }
        Row {
            id: copyRow
            //  Figure title
            Layout.fillWidth: true
            Text {
                width: copyRow.width - copyRow.spacing
                textFormat: Text.RichText
                text: "<b>" + wrapper.title + ":</b> " + wrapper.payload.description
                wrapMode: Text.WordWrap
            }
        }
    }
    FontMetrics {
        id: editorFM
        font: textArea.font
    }
}
