import QtQuick
import QtQuick.Controls

Rectangle {
    color: 'purple'
    signal addProject(string arch, string abstraction, string features)
    Button {
        text: "Create new project"
        anchors.centerIn: parent
        onClicked: addProject("pep/10", "isa", "")
    }
}
