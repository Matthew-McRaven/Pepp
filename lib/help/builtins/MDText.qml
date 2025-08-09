import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

TextArea {
    id: root
    required property string file
    property int architecture: Architecture.PEP10 // Silence QML warning about non-existent property
    signal navigateTo(string url)
    width: parent.width
    textFormat: file.endsWith("html") ? Text.RichText : Text.MarkdownText
    wrapMode: Text.WordWrap
    readOnly: true
    Component.onCompleted: {
        const content = FileReader.readFile(file);
        root.text = content;
    }
    onLinkActivated: link => {
        // # is an anchor in the current file, which I do not currently know hot to navigate to.
        if (link.startsWith("#")) {} else {
            root.navigateTo(link);
        }
    }
}
