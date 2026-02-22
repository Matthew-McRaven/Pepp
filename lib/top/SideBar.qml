import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Column {
    id: root
    required property string currentMode
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
    // Switching modesModel will cause our active mode to be unselected.
    // Instead, after the buttons after finished rendering, re-select the current mode.
    // TODO: handle the case where the current mode is not present in the new model.
    onModesModelChanged: {
        Qt.callLater(() => switchToMode(currentMode));
    }

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
        case "self test":
            return "robot.svg";
        }
    }
    Repeater {
        id: sidebarRepeater
        // If there is no current project, display a Welcome mode.
        model: root.modesModel ?? defaultSidebarModel
        delegate: Button {
            id: del
            property int borderSize: 2
            enabled: root.enabled
            checkable: true
            visible: settings.general.showDebugComponents || !(model.debugComponent ?? false)
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
                border.width: (del.hovered && !del.checked) ? del.borderSize : 0
                border.color: (del.hovered && !del.checked) ? palette.highlight : palette.shadow
                anchors.leftMargin: 4
                //  Show selected button color first. Show hover box second. Otherwise, use default
                color: del.checked ? palette.button.lighter(1.2) : palette.button
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
                radius: del.borderSize
            }
        }
    }

    // Make sidebar buttons mutually-exclusive.
    ButtonGroup {
        id: modeGroup
    }
}
