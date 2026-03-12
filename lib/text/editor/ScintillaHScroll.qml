import QtQuick
import QtQuick.Controls

ScrollBar {
    id: horizontalScrollBar
    required property var editor
    property real stepMultiple: 1.0
    property bool syncing: false
    property int visibleColumns: 0
    property int totalColumns: 0
    property int firstVisibleColumn: 0
    orientation: Qt.Horizontal

    height: horizontalScrollBar.visible ? 10 : 0
    visible: visibleColumns < totalColumns
    stepSize: stepMultiple / totalColumns
    size: visibleColumns / (totalColumns * 1.0)
    position: firstVisibleColumn / totalColumns
    function updatePosition() {
        if (!syncing) {
            // Qt paints stale content when we scroll if not bracketed by enableUpdate
            editor.enableUpdate(false);
            editor.scrollColumnAbsolute(Math.round(position * editor.totalColumns));
            editor.enableUpdate(true);
        }
    }

    onPositionChanged: updatePosition()
    function updateParams() {
        // Do not allow position to be updated are we re-compute, else we jump around the UI.
        syncing = true;
        if (editor.visibleColumns != horizontalScrollBar.visibleColumns) {
            horizontalScrollBar.visibleColumns = editor.visibleColumns;
        }
        if (editor.totalColumns != horizontalScrollBar.totalColumns) {
            horizontalScrollBar.totalColumns = editor.totalColumns;
        }
        if (editor.firstVisibleColumn != horizontalScrollBar.firstVisibleColumn) {
            horizontalScrollBar.firstVisibleColumn = editor.firstVisibleColumn;
        }
        syncing = false;
    }
    Component.onCompleted: updateParams()
    onWidthChanged: updateParams()

    Connections {
        target: horizontalScrollBar.editor
        function onModified() {
            horizontalScrollBar.updateParams();
        }
    }
}
