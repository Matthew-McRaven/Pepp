import QtQuick
import QtQuick.Controls
import edu.pepp 1.0

Rectangle {
    color: 'purple'
    signal addProject(string arch, string abstraction, string features)
    Button {
        text: "Create new project"
        anchors.centerIn: parent
        onClicked: addProject(Architecture.PEP10, Abstraction.ISA3, "")
    }
}
