import QtQuick
import QtQuick.Dialogs

MessageDialog {
    property int standardButtons: 0
    // Comments in qplatformdialoghelper.h indicate enumerations are supposed to be kept in sync.
    buttons: standardButtons
}
