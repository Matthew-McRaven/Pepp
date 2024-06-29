

/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
import QtQuick
//2.9
import QtQuick.Controls
//2.3
import QtQuick.Dialogs
//1.2
import QtQml.Models

//2.1
import org.scintilla.scintilla 1.0

Item {
    id: root

    property real charHeight: editor.charHeight

    focusPolicy: Qt.StrongFocus

    //filtersChildMouseEvents: false
    property alias quickScintillaEditor: editor

    // public properties
    property alias text: editor.text
    property alias readOnly: editor.readonly
    property alias editorFont: editor.font
    property alias language: editor.language
    // private properties, used only for technical details...
    property alias scintilla: editor
    property bool actionFromKeyboard: false

    property var menuCommandDelegate: undefined

    focus: true
    onFocusChanged: {
        quickScintillaEditor.focus = focus
    }

    WheelHandler {
        target: editor
        onWheel: function (event) {
            if (event.angleDelta.y > 0) {
                verticalScrollBar.decrease()
            } else if (event.angleDelta.y < 0) {
                verticalScrollBar.increase()
            }

            if (event.angleDelta.x > 0) {
                horizontalScrollBar.decrease()
            } else if (event.angleDelta.x < 0) {
                horizontalScrollBar.increase()
            }
        }
    }
    // the QuickScintilla control
    ScintillaEditBase {
        id: editor
        anchors.top: parent.top
        anchors.bottom: horizontalScrollBar.top
        anchors.left: parent.left
        anchors.right: verticalScrollBar.left

        readonly: false

        Accessible.role: Accessible.EditableText
        focus: true
    }
    ScrollBar {
        id: verticalScrollBar
        orientation: Qt.Vertical
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: editor.visibleLines < editor.totalLines
        width: verticalScrollBar.visible ? 10 : 0
        stepSize: 3.0 / editor.totalLines
        position: editor.firstVisibleLine / editor.totalLines
        onPositionChanged: {
            editor.enableUpdate(false)
            editor.scrollRowAbsolute(Math.round(position * editor.totalLines))
            editor.enableUpdate(true)
        }
    }

    ScrollBar {
        id: horizontalScrollBar
        orientation: Qt.Horizontal
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: editor.visibleColumuns < editor.totalColumns
        height: horizontalScrollBar.visible ? 20 : 0
        stepSize: 3.0 / editor.totalColumns
        position: editor.firstVisibleColumn / editor.totalColumns
    }
}
