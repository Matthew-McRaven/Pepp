import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    property alias input: inputArea.text
    property alias output: outputArea.text

    Label {
        text: "Input"
    }
    ScrollView {
        Layout.minimumHeight: 200
        Layout.margins: 2
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentHeight: Math.max(inputArea.contentHeight, height)
        TextArea {
            id: inputArea
            background: Rectangle {
                color: palette.base
                border.color: palette.text
                border.width: 1
            }
        }
    }

    Label {
        text: "Output"
    }
    // Ibid.
    ScrollView {
        Layout.minimumHeight: 200
        Layout.margins: 2
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentHeight: Math.max(outputArea.contentHeight, height)
        TextArea {
            id: outputArea
            readOnly: true
            background: Rectangle {
                color: palette.base
                border.color: palette.text
                border.width: 1
            }
        }
    }
}
