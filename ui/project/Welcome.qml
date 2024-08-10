import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    color: 'purple'
    signal addProject(string arch, string abstraction, string features, bool reuse)
    ScrollView {
        anchors.fill: parent
        GridLayout {
            id: layout
            columns: 4
            anchors.fill: parent
            anchors.centerIn: parent
            Text {
                text: "<b>Pep/10</b>"
                color: palette.text
                Layout.columnSpan: layout.columns
                Layout.preferredHeight: implicitHeight
            }
            WelcomeCard {
                text: "MC2, 1 byte bus"
                architecture: Architecture.PEP10
                abstraction: Abstraction.MC2
                enabled: false
            }
            WelcomeCard {
                text: "MC2, 2 byte bus"
                architecture: Architecture.PEP10
                abstraction: Abstraction.MC2
                enabled: false
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
                enabled: false
            }
            WelcomeCard {
                text: "OS4"
                architecture: Architecture.PEP10
                abstraction: Abstraction.OS4
                enabled: false
            }
            WelcomeCard {
                text: "ASMB5, full OS"
                architecture: Architecture.PEP10
                abstraction: Abstraction.ASMB5
            }
            Text {
                text: "<b>RISC-V</b>"
                color: palette.text
                Layout.columnSpan: layout.columns
                Layout.preferredHeight: implicitHeight
            }
            WelcomeCard {
                text: "Assembly Language, bare metal"
                architecture: Architecture.RISCV
                abstraction: Abstraction.ISA3
                enabled: false
            }
            Text {
                text: "<b>Pep/9</b>"
                color: palette.text
                Layout.columnSpan: layout.columns
                Layout.preferredHeight: implicitHeight
            }
            WelcomeCard {
                text: "MC2, 1 byte bus"
                architecture: Architecture.PEP9
                abstraction: Abstraction.MC2
                enabled: false
            }
            WelcomeCard {
                text: "MC2, 2 byte bus"
                architecture: Architecture.PEP9
                abstraction: Abstraction.MC2
                enabled: false
            }
            WelcomeCard {
                text: "ISA3"
                architecture: Architecture.PEP9
                abstraction: Abstraction.ISA3
                enabled: false
            }
            WelcomeCard {
                text: "OS4"
                architecture: Architecture.PEP9
                abstraction: Abstraction.OS4
                enabled: false
            }
            WelcomeCard {
                text: "ASMB5"
                architecture: Architecture.PEP9
                abstraction: Abstraction.ASMB5
                enabled: false
            }
        }
    }
}
