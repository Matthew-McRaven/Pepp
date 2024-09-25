import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import edu.peppx 1.0
import "../about" as About

Dialog {
    id: aboutDialog
    title: qsTr("About Pep/10")
    modal: true
    dim: false
    focus: true

    implicitWidth: 500
    implicitHeight: 800
    standardButtons: Dialog.Ok
    width: Math.min(implicitWidth * 1.1, parent.width * .5)
    height: Math.min(implicitHeight, parent.height * .75)
    About.About {
        anchors.fill: parent
    }
}
