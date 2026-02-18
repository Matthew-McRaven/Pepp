import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "qrc:/qt/qml/edu/pepp/utils" as Comp

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
            action: root.actions.build.loadObject
            visible: root.actions.build.loadObject.enabled
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
            action: root.actions.build.assemble
            visible: root.actions.build.assemble.enabled
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
            visible:  (currentProject?.enabledSteps ?? 0) & StepEnableFlags.StepOver
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
            visible: (currentProject?.enabledSteps ?? 0) & StepEnableFlags.StepInto
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
            visible: (currentProject?.enabledSteps ?? 0) & StepEnableFlags.StepOut
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
