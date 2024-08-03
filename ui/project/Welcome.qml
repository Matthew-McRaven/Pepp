import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    color: 'purple'
    signal addProject(string arch, string abstraction, string features, bool reuse)
    Text {
        text: "<b>Pep/10</b>"
        color: palette.text
    }
    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.centerIn: parent
        WelcomeCard {
            text: "MC2, 1 byte bus"
            architecture: Architecture.PEP10
            abstraction: Abstraction.MC2
        }
        WelcomeCard {
            text: "MC2, 2 byte bus"
            architecture: Architecture.PEP10
            abstraction: Abstraction.MC2
        }
        WelcomeCard {
            text: "ISA3, bare metal"
            architecture: Architecture.PEP10
            abstraction: Abstraction.ISA3
        }
        WelcomeCard {
            text: "Assembly Language, bare metal"
            architecture: Architecture.PEP10
            abstraction: Abstraction.ISA3
        }
        WelcomeCard {
            text: "OS4"
            architecture: Architecture.PEP10
            abstraction: Abstraction.OS4
        }
        WelcomeCard {
            text: "ASMB5, full OS"
            architecture: Architecture.PEP10
            abstraction: Abstraction.ASMB5
        }
    }
}
