import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Column {
    id: root
    property var modesModel: undefined
    function switchToMode(mode) {
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
        ListElement {
            display: "Dock"
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
        }
    }

    // Make sidebar buttons mutually-exclusive.
    ButtonGroup {
        id: modeGroup
    }
}
