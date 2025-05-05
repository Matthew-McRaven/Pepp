import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "." as Ui
import "qrc:/qt/qml/edu/pepp/components" as Comp
import Qt.labs.platform as Platform

Item {
    id: root
    // Can't name palette or it clashes with exsiting property.
    required property var ePalette
    required property var paletteRole
    required property var paletteItem
    required property bool isSystem
    required property bool isMonoFont
    implicitWidth: columnLayout.implicitWidth
    implicitHeight: columnLayout.implicitHeight

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
                Comp.DisableableComboBox {
                    id: parentCombo
                    model: ValidPaletteParentModel {
                        role: root.paletteRole ?? 0
                    }
                    enabled: !root.isSystem
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
                    enabled: !root.isSystem
                             && (root.paletteItem?.parent ?? false)
                    onPressed: {
                        root.paletteItem.clearParent()
                    }
                }
            }
        } // parentGB

        GroupBox {
            id: colorsGB
            Layout.fillWidth: true

            //  Used to size dynamic text fields
            TextMetrics {
                id: tm
                text: "Overriding parent value"
            }

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
                    enabled: !root.isSystem
                }
                CheckBox {
                    id: fgCheck
                    Layout.minimumWidth: tm.width + indicator.width + 30
                    enabled: !root.isSystem && !!root.paletteItem?.parent
                    text: !root.paletteItem?.parent ? "Using own value" : (checked ? "Overriding parent value" : "Using parent value")
                    checked: enabled && root.paletteItem?.hasOwnForeground
                    onCheckedChanged: {
                        if (enabled && !checked && root.paletteItem) {
                            root.paletteItem.clearForeground()
                        }
                        //console.log("Text " + tm.width)
                    }
                }
                Label {
                    text: "Background"
                }
                Ui.ColorButton {
                    id: bgPicker
                    color: root.paletteItem?.background ?? "black"
                    enabled: !root.isSystem
                }
                CheckBox {
                    id: bgCheck
                    enabled: !root.isSystem && !!root.paletteItem?.parent
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
                    enabled: !root.isSystem
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
                    enabled: !root.isSystem
                             && ((root.paletteItem?.parent
                                  && root.paletteItem?.hasOwnFont) ?? false)
                    onPressed: {
                        if (enabled)
                            root.paletteItem.clearFont()
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
                enabled: (!root.isSystem
                          && root.paletteItem) ? !root.paletteItem.hasOwnFont : false
            }
            GridLayout {
                id: layout
                columns: 2
                columnSpacing: 2
                rowSpacing: 2
                CheckBox {
                    text: "Bold"
                    enabled: (!root.isSystem
                              && root.paletteItem) ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.bold ?? false
                    onReleased: {
                        root.paletteItem.overrideBold(checked)
                    }
                }
                CheckBox {
                    text: "Italic"
                    enabled: (!root.isSystem
                              && root.paletteItem) ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.italic ?? false
                    onReleased: {
                        root.paletteItem.overrideItalic(checked)
                    }
                }
                CheckBox {
                    text: "Underline"
                    enabled: (!root.isSystem
                              && root.paletteItem) ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.underline ?? false
                    onReleased: {
                        root.paletteItem.overrideUnderline(checked)
                    }
                }
                CheckBox {
                    text: "Strikeout"
                    enabled: (!root.isSystem
                              && root.paletteItem) ? !root.paletteItem.hasOwnFont : false
                    checked: root.paletteItem?.font.strikeout ?? false
                    onReleased: {
                        root.paletteItem.overrideStrikeout(checked)
                    }
                }
                RowLayout {
                    Layout.columnSpan: 2
                    Label {
                        text: "Size"
                        enabled: !root.isSystem
                                 && (root.paletteItem ? !root.paletteItem.hasOwnFont : false)
                    }
                    SpinBox {
                        id: sizeSB
                        enabled: !root.isSystem
                                 && (root.paletteItem ? !root.paletteItem.hasOwnFont : false)
                        value: root.paletteItem?.font.pixelSize ?? 12
                        from: 1
                    }
                }
            } //  ColumnLayout
        } // fontOverrideGB

        GroupBox {
            id: macroFontGB
            visible: root.paletteItem?.clearMacroFont !== undefined
            RowLayout {
                Label {
                    text: "Macro Font "
                }
                Button {
                    enabled: !root.isSystem
                    text: root.paletteItem?.macroFont?.family ?? "Select Font"
                    font: root.paletteItem?.macroFont
                    onPressed: {
                        //  Open dialog and set properties.
                        macroFontDialog.open()
                        // Must explicitly update current font, because the binding is ignored.
                        macroFontDialog.open()
                        macroFontDialog.currentFont = root.paletteItem.macroFont
                        macroFontDialog.visible = true
                    }
                }
                Button {
                    text: "Reset to Parent"
                    enabled: !root.isSystem
                             && ((root.paletteItem?.parent
                                  && root.paletteItem?.hasOwnMacroFont)
                                 ?? false)
                    onPressed: {
                        if (enabled)
                            root.paletteItem.clearMacroFont()
                    }
                }
            }
        } // fontGB
        Item {
            Layout.fillHeight: true
        }
    }

    Platform.FontDialog {
        id: fontDialog
        options: root.isMonoFont ? Platform.FontDialog.MonospacedFonts : 0
        // Hack to create a default font.
        TextMetrics {
            id: defaultFont
        }
        currentFont: root.paletteItem?.font ?? defaultFont.font
        onAccepted: root.paletteItem.font = font
    }
    Platform.FontDialog {
        id: macroFontDialog
        options: Platform.FontDialog.MonospacedFonts
        currentFont: root.paletteItem?.macroFont ?? defaultFont.font
        onAccepted: root.paletteItem.macroFont = font
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
