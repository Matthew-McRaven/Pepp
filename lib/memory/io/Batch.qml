import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp

ColumnLayout {
    id: root
    NuAppSettings {
        id: settings
    }
    function forceFocusEditor() {
        area.forceActiveFocus();
    }
    property alias text: area.text
    property bool readOnly: false
    property real minimumHeight: scrollViewMinHeight
    property real scrollViewMinHeight: 100
    spacing: 0

    ScrollView {
        id: scrollView
        Layout.minimumHeight: root.scrollViewMinHeight
        Layout.margins: 0 //2
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentHeight: Math.max(area.contentHeight, height)
        TextArea {
            id: area
            font: settings.extPalette.baseMono.font
            background: Rectangle {
                color: palette.base
                border.color: palette.shadow
                border.width: 1
            }
            // Read-only editor is not selectable in WASM for... reasons.
            // So we need to fake it in software.
            readOnly: root.readOnly && !PlatformDetector.isWASM
            Keys.onPressed: function(e) {
                if(!PlatformDetector.isWASM || !root.readOnly) return;

                const k = e.key;
                const mods = e.modifiers;
                const ctrlOrMeta = (mods & Qt.ControlModifier) || (mods & Qt.MetaModifier);
                const shift = (mods & Qt.ShiftModifier) !== 0;

                // Allow copy shortcuts
                if (ctrlOrMeta && (k === Qt.Key_C || k === Qt.Key_Insert)) {
                    e.accepted = false;
                    return;
                }

                // Allow navigation keys (with or without Shift for selection)
                switch (k) {
                case Qt.Key_Left:
                case Qt.Key_Right:
                case Qt.Key_Up:
                case Qt.Key_Down:
                case Qt.Key_Home:
                case Qt.Key_End:
                case Qt.Key_PageUp:
                case Qt.Key_PageDown:
                    e.accepted = false;
                    return;
                }

                // Allow selection shortcuts (Ctrl+A, Shift+Home/End already covered by nav+shift)
                if (ctrlOrMeta && k === Qt.Key_A) {
                    e.accepted = false;
                    return;
                }

                // Block everything else that would edit (typing, backspace/delete, paste, enter, etc.)
                e.accepted = true;
            }
        }
    }
}
