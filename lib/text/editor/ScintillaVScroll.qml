import QtQuick
import QtQuick.Controls

ScrollBar {
    id: verticalScrollBar
    required property var editor
    property real stepMultiple: 1.0
    property int visibleLines: 0
    property int totalLines: 0
    property int firstVisibleLine: 0
    orientation: Qt.Vertical

    width: verticalScrollBar.visible ? 10 : 0
    visible: visibleLines < totalLines
    stepSize: stepMultiple / totalLines
    size: visibleLines / (totalLines * 1.0)
    position: firstVisibleLine / totalLines
    onPositionChanged: {
        // Qt paints stale content when we scroll if not bracketed by enableUpdate
        editor.enableUpdate(false);
        editor.scrollRowAbsolute(Math.round(position * editor.totalLines));
        editor.enableUpdate(true);
    }
    function updateParams() {
        if (editor.visibleLines != verticalScrollBar.visibleLines)
            verticalScrollBar.visibleLines = editor.visibleLines;
        if (editor.totalLines != verticalScrollBar.totalLines)
            verticalScrollBar.totalLines = editor.totalLines;
        if (editor.firstVisibleLine != verticalScrollBar.firstVisibleLine)
            verticalScrollBar.firstVisibleLine = editor.firstVisibleLine;
    }
    Component.onCompleted: updateParams()
    onHeightChanged: updateParams()

    Connections {
        target: verticalScrollBar.editor
        function onModified() {
            verticalScrollBar.updateParams();
        }
    }
}
