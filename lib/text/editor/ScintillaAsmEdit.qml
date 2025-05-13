/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQml.Models
import edu.pepp 1.0 as Pepp

FocusScope {
    id: root
    NuAppSettings {
        id: settings
    }

    signal editingFinished(string text)
    Component.onCompleted: {
        editor.onActiveFocusChanged.connect(function () {
            if (!editor.activeFocus)
                root.editingFinished(editor.text);
        });
    }
    // List has {line:#, message:str}
    function addEOLAnnotations(lst) {
        editor.clearAllEOLAnnotations();
        // See styles at: https://scintilla.org/ScintillaDoc.html#EndOfLineAnnotations
        let style = lst.length === 0 ? 0x0 : 0x2;
        editor.setEOLAnnotationsVisible(style);
        for (var i = 0; i < lst.length; i++) {
            editor.addEOLAnnotation(lst[i].line, lst[i].message);
        }
    }
    // List has {line:#, message:str}
    function addListingAnnotations(lst) {
        editor.clearAllInlineAnnotations();
        // See styles at: https://scintilla.org/ScintillaDoc.html#EndOfLineAnnotations
        let style = lst.length === 0 ? 0x0 : 0x1;
        editor.setInlineAnnotationsVisible(style);
        for (var i = 0; i < lst.length; i++) {
            editor.addInlineAnnotation(lst[i].line, lst[i].message);
        }
    }

    property real charHeight: editor.charHeight

    focusPolicy: Qt.StrongFocus

    //filtersChildMouseEvents: false
    property alias quickScintillaEditor: editor
    property alias editor: editor

    // public properties
    property alias text: editor.text
    property alias readOnly: editor.readOnly
    property alias editorFont: editor.font
    property alias language: editor.language
    // private properties, used only for technical details...
    property alias scintilla: editor
    property bool actionFromKeyboard: false

    property var menuCommandDelegate: undefined

    focus: true
    onFocusChanged: {
        editor.focus = focus;
    }

    WheelHandler {
        target: editor
        onWheel: function (event) {
            if (event.angleDelta.y > 0) {
                verticalScrollBar.decrease();
            } else if (event.angleDelta.y < 0) {
                verticalScrollBar.increase();
            }

            if (event.angleDelta.x > 0) {
                horizontalScrollBar.decrease();
            } else if (event.angleDelta.x < 0) {
                horizontalScrollBar.increase();
            }
        }
        acceptedDevices: PointerDevice.AllDevices
    }
    // the QuickScintilla control
    Pepp.ScintillaAsmEditBase {
        id: editor
        Component.onCompleted: {
            settings.extPalette.itemChanged.connect(editor.applyStyles);
        }
        anchors.top: parent.top
        anchors.bottom: horizontalScrollBar.top
        anchors.left: parent.left
        anchors.right: verticalScrollBar.left

        readOnly: false

        Accessible.role: Accessible.EditableText
        focus: true
        theme: settings.extPalette
        lineNumbersVisible: true
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
            editor.enableUpdate(false);
            editor.scrollRowAbsolute(Math.round(position * editor.totalLines));
            editor.enableUpdate(true);
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
