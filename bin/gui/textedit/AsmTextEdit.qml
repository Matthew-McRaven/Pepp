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
    LineInfoModel {
        id: lineModel
        document: editor.textDocument
    }

    RowLayout {
        spacing: 0
        anchors.fill: parent
        Layout.fillHeight: true
        Layout.fillWidth: true

        //  Line numbers
        Rectangle {
            id: rows
            width: wrapper.colWidth
            property real rowHeight: editor.lineHeight
            property real colWidth: wrapper.colWidth
            property real bulletSize: rowHeight * 0.75

            Layout.topMargin: editor.topPadding + editor.textMargin
            Layout.bottomMargin: editor.bottomPadding
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignLeft
            Layout.maximumWidth: rows.width
            Layout.minimumWidth: rows.width

            ListView {
                id: view
                anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom;
                width: colWidth;

                clip: false //true
                focus: false
                interactive: false

                model: lineModel
                delegate:
                    Rectangle {
                        id: row
                        //color: view.currentIndex === index ? root.highlightColor : root.backgroundColor
                        width: view ? ListView.view.width : 0
                        height: rows.rowHeight

                        required property int index
                        required property bool allowsBP
                        required property bool hasBP

                        Rectangle {
                            id: bullet
                            visible: row.hasBP
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            color: "red"
                            height: rows.bulletSize
                            width: rows.bulletSize
                            radius: rows.bulletSize / 2
                        }
                        MouseArea {
                            visible: row.allowsBP
                            anchors.fill: parent
                            onClicked: {
                                view.model.toggleBreakpoint(row.index);
                            }
                        }
                        Label {
                            id: rowNum
                            anchors.left: bullet.right; anchors.right: parent.right
                            anchors.top: parent.top; anchors.bottom: parent.bottom
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 5; rightPadding: 5
                            font.bold: view.currentIndex === row.index
                            text: row.index + 1
                        }
                    }
            }
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