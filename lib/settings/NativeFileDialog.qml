import QtCore
import QtQuick
import QtQuick.Dialogs

FileDialog {
    property string selectedNameFilter_index: ""
    property string mode: ""
    currentFolder: StandardPaths.standardLocations(
                       StandardPaths.AppConfigLocation)[0]
    onModeChanged: {
        switch (mode) {
        case "OpenFile":
            fileMode = FileDialog.OpenFile
            break
        case "OpenFiles":
            fileMode = FileDialog.OpenFiles
            break
        case "SaveFile":
            fileMode = FileDialog.SaveFile
            break
        }
    }
    onSelectedNameFilter_indexChanged: {
        selectedNameFilter.index = selectedNameFilter_index
    }
}
