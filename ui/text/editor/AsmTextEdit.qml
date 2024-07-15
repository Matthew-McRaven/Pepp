

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
Flickable {
    id: wrapper
    signal editingFinished(string text)
    required property bool isReadOnly
    property alias readOnly: wrapper.isReadOnly
    property bool allowsBP: true
    required property string edition
    required property string language
    property alias text: editor.text

    property int colWidth: 30
    property int rows: 16
    ScrollBar.vertical: ScrollBar {}
    property alias editorHeight: editor.implicitHeight
    // To force the flickable to consume all available space in parent (leaving no gray areas below it),
    // set `contentHeight: Math.max(editorHeight, parent.height)`.
    // Content height must be set or scrolling is not possible.
    contentHeight: editorHeight
    clip: true

    //  Set page contents based on parent selected values
    Component.onCompleted: {
        editor.editingFinished.connect(text => wrapper.editingFinished(text))
    }

    BlockFinder {
        id: finder
    }
    LineInfoModel {
        id: lineModel
        document: editor.textDocument
    }
    TabNanny {
        id: tabNanny
        document: editor.textDocument
    }

    //  Line numbers
    Rectangle {
        id: rows
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: wrapper.colWidth
        property real rowHeight: editor.lineHeight
        property real colWidth: wrapper.colWidth
        property real bulletSize: rowHeight * 0.75

        color: palette.window.darker(1.2)

        ListView {
            topMargin: editor.topPadding + editor.textMargin
            bottomMargin: editor.bottomPadding
            id: view
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: colWidth

            clip: false
            focus: false
            interactive: false

            model: lineModel
            delegate: Item {
                id: row
                width: view ? ListView.view.width : 0
                height: rows.rowHeight

                required property int index
                required property bool allowsBP
                required property bool hasBP
                required property bool hasNumber
                required property int number
                required property int errorState

                Rectangle {
                    id: bullet
                    visible: row.hasBP && wrapper.allowsBP
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.error.background
                    height: rows.bulletSize
                    width: rows.bulletSize
                    radius: rows.bulletSize / 2
                }
                MouseArea {
                    visible: row.allowsBP && wrapper.allowsBP
                    anchors.fill: parent
                    onClicked: {
                        view.model.toggleBreakpoint(row.index)
                    }
                }
                Label {
                    visible: row.hasNumber
                    id: rowNum
                    anchors.left: bullet.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 5
                    rightPadding: 5
                    font.bold: view.currentIndex === row.index
                    text: row.number
                    width: rows.bulletSize / 1.5
                }
                Rectangle {
                    id: warning
                    anchors.left: rowNum.right
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: row.errorState === 0 ? "transparent" : (row.errorState === 1 ? Theme.error.background : Theme.warning.background)
                }
            }
        }
    }
    // If I put TextArea directly in parent, parent shrinks to the size of the TextArea
    // This happens even when I try to force fillHeight / fillWidth in the containing Layout.
    // Nesting the editor defeats this behavior.
    Item {
        anchors.left: rows.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        TextArea {
            anchors.fill: parent
            Layout.alignment: Qt.AlignLeft
            id: editor
            textFormat: TextEdit.PlainText
            renderType: Text.NativeRendering
            property real lineHeight: contentHeight / lineCount
            font.family: "Courier New"
            background: Rectangle {
                color: palette.base
            }

            readOnly: wrapper.isReadOnly
        }

        function onPressedHandler(event) {
            if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
                if (editor.readOnly) {
                    event.accepted = true
                    return
                } else if (event.key === Qt.Key_Backtab
                           || (event.key === Qt.Key_Tab
                               && event.modifiers & Qt.ShiftModifier)) {
                    event.accepted = true
                    tabNanny.backtab(editor.cursorPosition)
                } else if (event.key === Qt.Key_Tab
                           && event.modifiers === Qt.NoModifier) {
                    event.accepted = true
                    tabNanny.tab(editor.cursorPosition)
                }
            }
        }
        Keys.onPressed: event => onPressedHandler(event)
    }
}
