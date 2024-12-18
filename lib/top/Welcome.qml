import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Flickable {
    signal addProject(string arch, string abstraction, string features, bool reuse)
    ScrollView {
        anchors.fill: parent
        GridLayout {
            id: layout
            columns: 1 + 6
            anchors.fill: parent
            anchors.centerIn: parent
            // Header
            Item {}

            Text {
                text: "ISA3"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Text {
                text: "Asmb3"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Text {
                text: "Asmb5"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Text {
                text: "OS4"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Text {
                text: "Mc2-1B"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Text {
                text: "Mc2-2B"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }

            // Row 1
            Text {
                text: "Pep/10"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                Layout.preferredHeight: implicitHeight
            }

            WelcomeCard {
                text: "Pep/10, ISA3, bare metal"
                architecture: Architecture.PEP10
                abstraction: Abstraction.ISA3
                source: "image://icons/cards/p10_isa3.svg"
                description: "Develop and debug machine language programs in bare metal mode."
            }
            WelcomeCard {
                text: "Pep/10, Asmb3, bare metal"
                architecture: Architecture.PEP10
                abstraction: Abstraction.ASMB3
                source: "image://icons/cards/p10_asmb3.svg"
                description: "Develop and debug assembly language programs in bare metal mode."
            }

            WelcomeCard {
                text: "Pep/10, Asmb5, full OS"
                architecture: Architecture.PEP10
                abstraction: Abstraction.ASMB5
                source: "image://icons/cards/p10_asmb5.svg"
                description: "Develop and debug assembly language programs alongside Pep/10's operating system. This level enables you to utilize OS features using system calls for enhanced functionality."
            }
            WelcomeCard {
                text: "Pep/10, OS4"
                architecture: Architecture.PEP10
                abstraction: Abstraction.OS4
                enabled: false
            }
            WelcomeCard {
                text: "Pep/10, Mc2, 1-byte bus"
                architecture: Architecture.PEP10
                abstraction: Abstraction.MC2
                source: "image://icons/cards/p10_mc2_1byte.svg"
                enabled: false
            }
            WelcomeCard {
                text: "Pep/10, Mc2, 2-byte bus"
                architecture: Architecture.PEP10
                abstraction: Abstraction.MC2
                enabled: false
            }
            // Row 2
            Text {
                text: "Pep/9"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                Layout.preferredHeight: implicitHeight
            }

            WelcomeCard {
                text: "Pep/9, ISA3"
                architecture: Architecture.PEP9
                abstraction: Abstraction.ISA3
                enabled: true
                source: "image://icons/cards/p9_isa3.svg"
                description: "Develop and debug machine language programs."
            }
            Item {}

            WelcomeCard {
                text: "Pep/9, Asmb5"
                architecture: Architecture.PEP9
                abstraction: Abstraction.ASMB5
                enabled: true
                source: "image://icons/cards/p9_asmb5.svg"
                description: "Develop and debug assembly language programs alongside Pep/9's operating system. This level enables you to utilize OS features using trap instructions for enhanced functionality."
            }
            WelcomeCard {
                text: "Pep/9, OS4"
                architecture: Architecture.PEP9
                abstraction: Abstraction.OS4
                enabled: false
            }
            WelcomeCard {
                text: "Pep/9, Mc2, 1-byte bus"
                architecture: Architecture.PEP9
                abstraction: Abstraction.MC2
                source: "image://icons/cards/p10_mc2_1byte.svg"
                enabled: false
            }
            WelcomeCard {
                text: "Pep/9, Mc2, 2-byte bus"
                architecture: Architecture.PEP9
                abstraction: Abstraction.MC2
                enabled: false
            }
            // Row 3
            Text {
                text: "Pep/8"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                Layout.preferredHeight: implicitHeight
            }

            WelcomeCard {
                text: "Pep/8, ISA3"
                architecture: Architecture.PEP8
                abstraction: Abstraction.ISA3
                enabled: false
                source: "image://icons/cards/p9_isa3.svg"
            }
            Item {}
            WelcomeCard {
                text: "Pep/8, Asmb5"
                architecture: Architecture.PEP8
                abstraction: Abstraction.ASMB5
                enabled: false
                source: "image://icons/cards/p9_asmb5.svg"
            }
            WelcomeCard {
                text: "Pep/8, OS4"
                architecture: Architecture.PEP8
                abstraction: Abstraction.OS4
                enabled: false
            }

            WelcomeCard {
                text: "Pep/8, Mc2, 1-byte bus"
                architecture: Architecture.PEP8
                abstraction: Abstraction.MC2
                source: "image://icons/cards/p10_mc2_1byte.svg"
                enabled: false
            }
            Item {}
            // Row 4
            Text {
                text: "RISC-V"
                font.pointSize: 24
                font.weight: Font.Bold
                color: palette.text
                Layout.preferredHeight: implicitHeight
            }
            Item {}
            WelcomeCard {
                Layout.alignment: Qt.AlignRight
                text: "RISC-V, Asmb3, bare metal"
                architecture: Architecture.RISCV
                abstraction: Abstraction.ASMB3
                enabled: false
            }
            Item {}

            Item {}
            Item {}
            Item {}
        }
    }
}
