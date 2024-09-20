import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {

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
                text: "Pep/10, Mc2, 1-byte bus"
                architecture: Architecture.PEP10
                abstraction: Abstraction.MC2
                enabled: false
            }
            WelcomeCard {
                text: "Pep/10, Mc2, 2-byte bus"
                architecture: Architecture.PEP10
                abstraction: Abstraction.MC2
                enabled: false
            }
            WelcomeCard {
                text: "Pep/10, ISA3, bare metal"
                architecture: Architecture.PEP10
                abstraction: Abstraction.ISA3
                source: "image://icons/cards/p10_isa3.svg"
            }
            WelcomeCard {
                text: "Pep/10, Asmb3, bare metal"
                architecture: Architecture.PEP10
                abstraction: Abstraction.ASMB3
                source: "image://icons/cards/p10_asmb3.svg"
                enabled: true
            }
            WelcomeCard {
                text: "Pep/10, OS4"
                architecture: Architecture.PEP10
                abstraction: Abstraction.OS4
                enabled: false
            }
            WelcomeCard {
                text: "Pep/10, Asmb5, full OS"
                architecture: Architecture.PEP10
                abstraction: Abstraction.ASMB5
                source: "image://icons/cards/p10_asmb5.svg"
            }
            Text {
                text: "<b>RISC-V</b>"
                color: palette.text
                Layout.columnSpan: layout.columns
                Layout.preferredHeight: implicitHeight
            }
            WelcomeCard {
                text: "RISC-V, Asmb3, bare metal"
                architecture: Architecture.RISCV
                abstraction: Abstraction.ASMB3
                enabled: false
            }
            Text {
                text: "<b>Pep/9</b>"
                color: palette.text
                Layout.columnSpan: layout.columns
                Layout.preferredHeight: implicitHeight
            }
            WelcomeCard {
                text: "Pep/9, Mc2, 1-byte bus"
                architecture: Architecture.PEP9
                abstraction: Abstraction.MC2
                enabled: false
            }
            WelcomeCard {
                text: "Pep/9, Mc2, 2-byte bus"
                architecture: Architecture.PEP9
                abstraction: Abstraction.MC2
                enabled: false
            }
            WelcomeCard {
                text: "Pep/9, ISA3"
                architecture: Architecture.PEP9
                abstraction: Abstraction.ISA3
                enabled: false
                source: "image://icons/cards/p9_isa3.svg"
            }
            WelcomeCard {
                text: "Pep/9, OS4"
                architecture: Architecture.PEP9
                abstraction: Abstraction.OS4
                enabled: false
            }
            WelcomeCard {
                text: "Pep/9, Asmb5"
                architecture: Architecture.PEP9
                abstraction: Abstraction.ASMB5
                enabled: false
                source: "image://icons/cards/p9_asmb5.svg"
            }
        }
    }
}
