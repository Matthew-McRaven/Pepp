import QtCore
import QtQuick
import QtQuick.Dialogs

FolderDialog {
    currentFolder: StandardPaths.standardLocations(
        StandardPaths.AppConfigLocation)[0]
}
