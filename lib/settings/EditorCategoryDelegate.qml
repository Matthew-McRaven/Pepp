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
    ScrollView {
        id: flickable
        anchors {
            fill: parent
            margins: 2 * bg.border.width
            leftMargin: 4 * anchors.margins
        }
        ScrollBar.vertical.policy: flickable.contentHeight
                                   > flickable.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: flickable.contentWidth
                                     > flickable.width ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded

        ColumnLayout {
            GroupBox {
                label: GroupBoxLabel {
                    text: qsTr("Code Editor Settings")
                    backgroundColor: bg.color
                }
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    CheckBox {
                        id: visualizeWhitespaceCheck
                        Component.onCompleted: checked = category.visualizeWhitespace
                        onCheckedChanged: category.visualizeWhitespace = checked
                        Connections {
                            target: category
                            function onVisualizeWhitespaceChanged() {
                                visualizeWhitespaceCheck.checked = category.visualizeWhitespace
                            }
                        }
                    }
                    Label {
                        text: "Visualize whitespace"
                    }
                }
            }
            Item {
                Layout.fillHeight: true
            }
        }
    }
}
