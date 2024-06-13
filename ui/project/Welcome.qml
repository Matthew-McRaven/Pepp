import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    color: 'purple'
    signal addProject(string arch, string abstraction, string features)
    RowLayout {
        anchors.fill: parent
        anchors.centerIn: parent
        Button {
            text: "Create ISA project"
            Layout.alignment: Qt.AlignHCenter
            onClicked: addProject(Architecture.PEP10, Abstraction.ISA3, "")
        }
        Button {
            text: "Create ASM project"
            Layout.alignment: Qt.AlignHCenter
            onClicked: addProject(Architecture.PEP10, Abstraction.ASMB5, "")
        }
    }
}
