import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    property alias input: inputArea.text
    property alias output: outputArea.text
    Label {
        text: "Input"
    }
    TextArea {
        id: inputArea
        background: Rectangle {
            color: "red"
        }
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
    Label {
        text: "Output"
    }
    TextArea {
        id: outputArea
        readOnly: true
        background: Rectangle {
            color: "blue"
        }
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
