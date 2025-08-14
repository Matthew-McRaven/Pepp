import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls
import edu.pepp

ColumnLayout {
    id: root
    spacing: 0
    Button {
        text: "Clear Messages"
        onClicked: {
            logTextArea.text = "";
        }
    }
    TextArea {
        id: logTextArea
        Layout.fillWidth: true
        Layout.fillHeight: true
        readOnly: true
        wrapMode: Text.Wrap
        font.family: "Monospace"
        textFormat: Text.RichText
    }

    function appendMessage(message) {
        TextDocumentAppender.appendText(logTextArea.textDocument, message);
    }
}
