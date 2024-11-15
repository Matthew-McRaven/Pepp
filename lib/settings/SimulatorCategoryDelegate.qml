import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import edu.pepp 1.0

Item {
    id: root
    property var category: undefined
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
    Flickable {
        anchors {
            fill: parent
            margins: 2 * bg.border.width
            leftMargin: 4 * anchors.margins
        }

        ColumnLayout {
            GroupBox {
                label: GroupBoxLabel {
                    text: qsTr("Tracing Options")
                    backgroundColor: bg.color
                }
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    SpinBox {
                        from: category?.minMaxStepbackBufferKB() ?? 10
                        to: category?.maxMaxStepbackBufferKB() ?? 20
                        value: category.maxStepbackBufferKB
                        // Step size should be ~10% of the value
                        stepSize: Math.pow(10, Math.floor(Math.log10(value)))
                        onValueModified: {
                            category.maxStepbackBufferKB = value
                        }
                    }
                    Label {
                        text: qsTr("Max simulator stepback buffer size (KB)")
                    }
                }
            }
        }
    }
}
