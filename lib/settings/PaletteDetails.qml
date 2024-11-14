import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "." as Ui
import Qt.labs.platform as Platform

Item {
    id: root
    // Can't name palette or it clashes with exsiting property.
    required property var ePalette
    required property var paletteRole
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
                    id: parentCombo
                    model: ValidPaletteParentModel {
                        role: root.paletteRole ?? 0
                    }
                    textRole: "display"
                    valueRole: "id"
                    currentIndex: root.ePalette.itemToRole(
                                      root.paletteItem?.parent)
                    onActivated: {
                        const parent = root.ePalette.item(currentIndex)
                        root.paletteItem.parent = parent
                    }
                }
                Button {
                    text: "Clear Parent"
                    enabled: root.paletteItem?.parent
                    onPressed: {
                        root.paletteItem.clearParent()
                    }
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
                    id: fgCheck
                    enabled: !!root.paletteItem?.parent
                    checked: enabled && root.paletteItem?.hasOwnForeground
                    onCheckedChanged: {
                        if (enabled && !checked && root.paletteItem)
                            root.paletteItem.clearForeground()
                    }
                }
                Label {
                    text: !root.paletteItem?.parent ? "Using own value" : (fgCheck.checked ? "Overriding parent value" : "Using parent value")
                    enabled: root.paletteItem?.parent
                }
                Label {
                    text: "Background"
                }
                Ui.ColorButton {
                    id: bgPicker
                    color: root.paletteItem?.background ?? "black"
                }
                CheckBox {
                    id: bgCheck
                    enabled: !!root.paletteItem?.parent
                    checked: enabled && root.paletteItem?.hasOwnBackground
                    onCheckedChanged: {
                        if (enabled && !checked && root.paletteItem)
                            root.paletteItem.clearBackground()
                    }
                }
                Label {
                    text: !root.paletteItem?.parent ? "Using own value" : (bgCheck.checked ? "Overriding parent value" : "Using parent value")
                    enabled: root.paletteItem?.parent
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
