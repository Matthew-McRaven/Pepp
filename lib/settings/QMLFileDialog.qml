import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    NuAppSettings {
        id: settings
    }

    required property string mode
    required property var nameFilters
    required property int selectedNameFilter_index
    // Used to trick import into working correctly
    property string selectedFile: ""
    // Unused, but needed for compatibility with Native dialog
    required property string title
    required property string defaultSuffix
    WASMIO {
        id: io
        onLoaded: {
            root.selectedFile = Qt.binding(() => io.loadedName)
            root.accepted()
        }
    }
    function open() {
        if (root.mode === "SaveFile")
            io.save("default.theme", settings.extPalette.jsonString())
        else if (root.mode === "OpenFile")
            io.load(root.nameFilters[root.selectedNameFilter_index])
    }
    signal accepted
}
