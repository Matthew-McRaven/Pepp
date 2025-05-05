import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

Flickable {
    required property string file
    Component.onCompleted: {
        const content = FileReader.readFile(file)
        textArea.text = content
    }
    ScrollBar.vertical: ScrollBar {}
    property alias editorHeight: textArea.implicitHeight
    // To force the flickable to consume all available space in parent (leaving no gray areas below it),
    // set `contentHeight: Math.max(editorHeight, parent.height)`.
    // Content height must be set or scrolling is not possible.
    contentHeight: Math.max(editorHeight, parent.height)
    TextArea {
        id: textArea
        anchors.fill: parent
        width: parent.width
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
        readOnly: true
    }
}
