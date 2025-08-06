import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

TextArea {
    id: root
    required property string file
    property int architecture: Architecture.PEP10 // Silence QML warning about non-existent property

    width: parent.width
    textFormat: Text.MarkdownText
    wrapMode: Text.WordWrap
    readOnly: true
    Component.onCompleted: {
        const content = FileReader.readFile(file);
        root.text = content;
    }
}
