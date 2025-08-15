import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls
import edu.pepp

ColumnLayout {
    id: root
    spacing: 0
    focus: true
    Button {
        text: "Clear Messages"
        onClicked: {
            logTextArea.text = "";
        }
    }
    ScrollView {
        id: view
        Layout.fillWidth: true
        Layout.fillHeight: true
        TextArea {
            id: logTextArea
            readOnly: true
            wrapMode: Text.Wrap
            font.family: "Monospace"
            textFormat: Text.RichText
        }
        ScrollBar.vertical.policy: logTextArea.contentHeight > view.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
    }

    function appendMessage(message) {
        TextDocumentAppender.appendText(logTextArea.textDocument, message);
    }
}
