import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Column {
    id: root
    property var modesModel: undefined
    function switchToMode(mode) {
        if (!root.enabled) {
            console.warn("Cannot switch modes when sidebar is disabled.");
            return;
        }
        // Match the button, case insensitive.
        const re = new RegExp(mode, "i");
        // Children of sidebar are the repeater's delegates
        for (var button of root.children) {
            if (re.test(button.text)) {
                button.clicked();
                // Must set checked in order for ButtonGroup to match current mode.
                button.checked = true;
                return;
            }
        }
        console.error(`Did not find mode ${mode}`);
    }
    signal modeChanged(string mode)

    ListModel {
        id: defaultSidebarModel
        ListElement {
            display: "Welcome"
        }
        ListElement {
            display: "Help"
        }
    }
    function mapModeToImage(mode) {
        switch (mode) {
        case "welcome":
            return "home.svg";
        case "help":
            return "help.svg";
        case "debugger":
            return "pest_control.svg";
        case "editor":
            return "edit.svg";
        }
    }
    Repeater {
        id: sidebarRepeater
        // If there is no current project, display a Welcome mode.
        model: root.modesModel ?? defaultSidebarModel
        delegate: Button {
            id: del
            property int borderSize: 1
            enabled: root.enabled
            checkable: true
            width: 100
            height: 65
            text: model.display ?? "ERROR"
            ButtonGroup.group: modeGroup
            onClicked: root.modeChanged(text.toLowerCase())
            icon.source: `image://icons/modes/${root.mapModeToImage(text.toLowerCase())}`
            icon.height: 42
            icon.width: 42
            display: AbstractButton.TextUnderIcon

            background: Rectangle {
                border.width: del.borderSize
                border.color: palette.shadow
                anchors.leftMargin: 4
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: del.down ? palette.highlight : (del.checked ? palette.button.darker(1.3) : palette.light.lighter(1.1))
                    }
                    GradientStop {
                        position: 1
                        color: del.down ? palette.highlight : (del.checked ? palette.button.darker(1.4) : palette.light.darker(1.1))
                    }
                }
            }
            Rectangle {
                visible: del.checked
                anchors.left: del.background.left
                anchors.top: del.background.top
                anchors.bottom: del.background.bottom
                anchors.margins: 0
                anchors.topMargin: del.borderSize
                anchors.bottomMargin: del.borderSize
                width: 4
                color: palette.highlight
            }
        }
    }

    // Make sidebar buttons mutually-exclusive.
    ButtonGroup {
        id: modeGroup
    }
}
