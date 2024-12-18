import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: wrapper
    required property int initialValue
    property alias mnemonics: mnemonic.model
    property alias font: metrics.font
    property int value: initialValue
    function setValue(value) {
        if (value >= 0 && value <= 255)
            wrapper.value = value
    }

    height: childrenRect.height

    FontMetrics {
        id: metrics
    }

    Row {
        TextField {
            id: dec
            text: `${wrapper.value}`
            maximumLength: 4
            width: metrics.averageCharacterWidth * maximumLength * 1.5
            font: metrics.font
            onEditingFinished: {
                wrapper.setValue(parseInt(dec.text, 10))
            }
            validator: RegularExpressionValidator {
                regularExpression: /(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)/
            }
            ToolTip.text: qsTr("Signed decimal")
            ToolTip.visible: hovered
        }
        TextField {
            id: hex
            text: `0x${wrapper.value.toString(16).padStart(2, '0')}`
            maximumLength: 4
            width: metrics.averageCharacterWidth * maximumLength * 1.5
            font: metrics.font
            onEditingFinished: {
                wrapper.setValue(parseInt(hex.text, 16))
            }
            validator: RegularExpressionValidator {
                regularExpression: /0x[0-9a-fA-F]{1,2}/
            }
            ToolTip.text: qsTr("Hexadecimal")
            ToolTip.visible: hovered
        }
        TextField {
            id: bin
            text: `${wrapper.value.toString(2).padStart(8, '0')}`
            maximumLength: 8
            width: metrics.averageCharacterWidth * maximumLength * 1.5
            font: metrics.font
            onEditingFinished: {
                wrapper.setValue(parseInt(bin.text, 2))
            }
            validator: RegularExpressionValidator {
                regularExpression: /[01]{1,8}/
            }
            ToolTip.text: qsTr("Binary")
            ToolTip.visible: hovered
        }
        TextField {
            CharCheck {
                id: check
            }
            id: ascii
            text: check.isCharSupported(String.fromCharCode(wrapper.value),
                                        ascii.font) ? String.fromCharCode(
                                                          wrapper.value) : "�"
            maximumLength: 1
            width: metrics.averageCharacterWidth * maximumLength * 4
            font: check.noMerge(metrics.font)
            onEditingFinished: {
                wrapper.setValue(ascii.text.charCodeAt(0) ?? 0)
            }
            ToolTip.text: qsTr("ASCII")
            ToolTip.visible: hovered
        }
        ComboBox {
            id: mnemonic
            textRole: 'display'
            // Disable if model is not set.
            visible: model ?? false
            // 7=>max mnemonic length, 2=>", ", 3=>max addr mode length
            width: metrics.averageCharacterWidth * (7 + 2 + 3) * 1.5
            font: metrics.font
            flat: true
            currentIndex: model?.indexFromOpcode(wrapper.value) ?? -1
            // Force mnemonic to re-compute current index by re-setting value.
            onModelChanged: {
                popup.contentItem.implicitHeight = Qt.binding(function () {
                    // According to <QtRoot>/qml/QtQuick/Controls/Basic/ComboBox.qml:
                    // This computation gets the height of the nested list view.
                    const actualContentHeight = popup.contentItem.contentHeight
                                              - popup.topMargin - popup.bottomMargin
                    // Prevent divide-by-0, since it would prevent execution of the rest of this fn.
                    const numRows = model?.rowCount() ?? 1
                    const actualRowHeight = actualContentHeight / numRows
                    return actualRowHeight * 10
                })
                wrapper.setValue(wrapper.value)
            }
            // Use on-activated to avoid binding loop from onCurrentIndexChanged.
            // activated only triggers on user interaction, wherease currentIndexChanged
            // triggers on any (programatic) update to currentIndex.
            onActivated: {
                wrapper.setValue(model.opcodeFromIndex(currentIndex))
            }
            ToolTip.text: qsTr("Opcode")
            ToolTip.visible: hovered
        }
    }
}
