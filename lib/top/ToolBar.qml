import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "qrc:/qt/qml/edu/pepp/components" as Comp

ToolBar {
    id: root
    required property var actions
    property int iconHeight: 30
    RowLayout {
        anchors.fill: parent
        ToolButton {
            action: root.actions.file.new_
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolSeparator {}
        ToolButton {
            visible: root.actions.build.assemble.enabled
            action: root.actions.build.assemble
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }

        ToolButton {
            action: root.actions.build.execute
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolButton {
            action: root.actions.debug.start
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolSeparator {}
        ToolButton {
            action: root.actions.debug.continue_
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolButton {
            action: root.actions.debug.stop
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolButton {
            action: root.actions.debug.step
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolButton {
            action: root.actions.debug.stepOver
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolButton {
            action: root.actions.debug.stepInto
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolButton {
            action: root.actions.debug.stepOut
            icon {
                source: action.icon.source
                height: root.iconHeight
                width: root.iconHeight
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: action.text.replace(/&/g, "")
            text: ''
        }
        ToolSeparator {}
        Comp.Converters {
            initialValue: 'a'.charCodeAt(0)
            mnemonics: currentProject?.mnemonics ?? null
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
