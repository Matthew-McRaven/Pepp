import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "." as Ui
import Qt.labs.platform as Platform

Item {
    id: root
    required property var paletteItem
    Rectangle {
        id: bg
        color: palette.base
        anchors {
            fill: parent
            margins: border.width
        }
        border {
            color: palette.text
            width: 1
        }
    }

    ColumnLayout {
        id: columnLayout
        anchors {
            fill: parent
            margins: 2 * bg.border.width
            leftMargin: 4 * anchors.margins
            rightMargin: 4 * anchors.margins
        }
        GroupBox {
            id: parentGB
            Layout.fillWidth: true
            label: GroupBoxLabel {
                textColor: palette.windowText
                backgroundColor: bg.color
                text: "Parent"
            }
            RowLayout {
                Label {
                    text: "Parent Item"
                }
                ComboBox {
                    model: ["a", "b", "c"]
                }
            }
        } // parentGB

        GroupBox {
            id: colorsGB
            Layout.fillWidth: true
            //  Groupbox label
            label: GroupBoxLabel {
                textColor: palette.windowText
                backgroundColor: bg.color
                text: "Colors"
            }
            GridLayout {
                columns: 4
                Label {
                    text: "Foreground"
                }
                Ui.ColorButton {
                    id: fgPicker
                    color: root.paletteItem?.foreground ?? "white"
                }
                CheckBox {
                    checked: root.paletteItem?.hasOwnForeground ?? false
                    onCheckedChanged: {
                        if (!checked && root.paletteItem)
                            root.paletteItem.clearForeground()
                    }
                }
                Label {
                    text: "Override parent"
                }
                Label {
                    text: "Background"
                }
                Ui.ColorButton {
                    id: bgPicker
                    color: root.paletteItem?.background ?? "black"
                }
                CheckBox {
                    checked: root.paletteItem?.hasOwnBackground ?? false
                    onCheckedChanged: {
                        if (!checked && root.paletteItem)
                            root.paletteItem.clearBackground()
                    }
                }
                Label {
                    text: "Override parent"
                }
            }
        } // colorsGB
        Item {
            Layout.fillHeight: true
        }
    }

    Platform.FontDialog {
        id: fontDialog

        onAccepted: {

        }
    }
    Platform.ColorDialog {
        id: colorDialog
        options: Platform.ColorDialog.ShowAlphaChannel
        property var setCallback: color => {}
        function onRequestBackground(_color) {
            if (_color)
                currentColor = Qt.binding(() => _color)
            setCallback = newColor => {
                root.paletteItem.background = newColor
            }
            colorDialog.open()
        }
        function onRequestForeground(_color) {
            if (_color)
                currentColor = Qt.binding(() => _color)
            setCallback = newColor => {
                root.paletteItem.foreground = newColor
            }
            colorDialog.open()
        }
        //  Signal parent control that color has changed
        onAccepted: {
            setCallback(colorDialog.color)
            setCallback = color => {}
        }
    }
    Component.onCompleted: {
        fgPicker.requestColorChange.connect(colorDialog.onRequestForeground)
        bgPicker.requestColorChange.connect(colorDialog.onRequestBackground)
    }
}
