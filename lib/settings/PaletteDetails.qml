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
                columns: 3
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
                    text: !root.paletteItem?.parent ? "Using own value" : (checked ? "Overriding parent value" : "Using parent value")
                    checked: enabled && root.paletteItem?.hasOwnForeground
                    onCheckedChanged: {
                        if (enabled && !checked && root.paletteItem)
                            root.paletteItem.clearForeground()
                    }
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
                    text: !root.paletteItem?.parent ? "Using own value" : (checked ? "Overriding parent value" : "Using parent value")
                    checked: enabled && root.paletteItem?.hasOwnBackground
                    onCheckedChanged: {
                        if (enabled && !checked && root.paletteItem)
                            root.paletteItem.clearBackground()
                    }
                }
            }
        } // colorsGB
        GroupBox {
            id: fontGB
            RowLayout {
                Label {
                    text: "Font "
                }
                Button {
                    text: root.paletteItem?.font.family
                    font: root.paletteItem?.font
                    onPressed: {
                        //  Open dialog and set properties.
                        fontDialog.open()
                        // Must explicitly update current font, because the binding is ignored.
                        fontDialog.open()
                        fontDialog.currentFont = root.paletteItem.font
                        fontDialog.visible = true
                    }
                }
                Button {
                    text: "Reset to Parent"
                    enabled: root.paletteItem?.parent
                             && root.paletteItem?.hasOwnFont
                    onPressed: {
                        if (enabled)
                            root.paletteItem.resetFont()
                    }
                }
            }
        } // fontGB

        GroupBox {
            id: fontOverrideGB
            Layout.fillWidth: true
            //  Groupbox label
            label: GroupBoxLabel {
                textColor: palette.windowText
                backgroundColor: bg.color
                text: "Font Overrides"
                enabled: root.paletteItem ? !root.paletteItem.hasOwnFont : false
            }
            GridLayout {
                id: layout
                columns: 2
                columnSpacing: 2
                rowSpacing: 2
                CheckBox {
                    text: "Bold"
                    enabled: root.paletteItem ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.bold
                    onCheckedChanged: {
                        if (enabled)
                            root.paletteItem.overrideBold(checked)
                    }
                }
                CheckBox {
                    text: "Italic"
                    enabled: root.paletteItem ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.italic
                    onCheckedChanged: {
                        if (enabled)
                            root.paletteItem.overrideItalic(checked)
                    }
                }
                CheckBox {
                    text: "Underline"
                    enabled: root.paletteItem ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.underline
                    onCheckedChanged: {
                        if (enabled)
                            root.paletteItem.overrideUnderline(checked)
                    }
                }
                CheckBox {
                    text: "Strikeout"
                    enabled: root.paletteItem ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.strikeout
                    onCheckedChanged: {
                        if (enabled)
                            root.paletteItem.overrideStrikeout(checked)
                    }
                }
                RowLayout {
                    Layout.columnSpan: 2
                    Label {
                        text: "Size"
                        enabled: root.paletteItem ? !root.paletteItem.hasOwnFont : false
                    }
                    SpinBox {
                        id: sizeSB
                        enabled: root.paletteItem ? !root.paletteItem.hasOwnFont : false
                        value: root.paletteItem?.font.pixelSize
                        from: 1
                    }
                }
            } //  ColumnLayout
        } // fontOverrideGB

        Item {
            Layout.fillHeight: true
        }
    }

    Platform.FontDialog {
        id: fontDialog
        currentFont: root.paletteItem.font
        onAccepted: {
            root.paletteItem.font = font
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
