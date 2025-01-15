import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    NuAppSettings {
        id: settings
    }
    ArchitectureUtils {
        id: utils
    }

    FontMetrics {
        id: fm
        font.pointSize: 48
    }
    signal addProject(string arch, string abstraction, string features, bool reuse)
    signal goBack
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            spacing: 20
            Button {
                text: "Choose Different Architecture"
                onReleased: root.goBack()
                icon.source: "image://icons/navigation/arrow_back.svg"
                display: AbstractButton.TextBesideIcon
            }
            Label {
                Layout.fillWidth: true
                text: `Choose a project for ${utils.archAsString(
                          settings.general.defaultArch)}`
                font: fm.font
            }
        }

        ListView {
            id: list
            model: elements
            Layout.fillWidth: true
            Layout.fillHeight: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            spacing: 0
            leftMargin: 20
            topMargin: 20
            bottomMargin: 20
            reuseItems: false
            // Because I do not have a filter model, I need selective spacing between cards.
            // This is an ugly hack, but it provides a proof-of-concept.
            delegate: Item {
                required property var model
                // Horrible hack until I have a real C++ model backing this
                implicitHeight: inner.visible ? 220 : 0
                implicitWidth: inner.visible ? 165 : 0
                WelcomeCard {
                    id: inner
                    anchors.fill: parent
                    anchors.topMargin: visible ? 10 : 0
                    anchors.bottomMargin: visible ? 10 : 0
                    visible: enabled
                             && (model.architecture === settings.general.defaultArch)
                    text: model.text
                    architecture: model.architecture
                    abstraction: model.abstraction
                    enabled: model.complete
                             || (model.partiallyComplete
                                 && settings.general.showDebugComponents)
                    source: model.source
                    description: model.description
                }
            }
        }
    }
    ListModel {
        id: elements
        // Pep/10
        ListElement {
            text: "ISA3, bare metal"
            architectures: Architecture.PEP10
            abstraction: Abstraction.ISA3
            source: "image://icons/cards/p10_isa3.svg"
            complete: true
            description: "Develop and debug machine language programs in bare metal mode."
        }
        ListElement {
            text: "Asmb3, bare metal"
            architecture: Architecture.PEP10
            abstraction: Abstraction.ASMB3
            source: "image://icons/cards/p10_asmb3.svg"
            complete: true
            description: "Develop and debug assembly language programs in bare metal mode."
        }
        ListElement {
            text: "Asmb5, full OS"
            architecture: Architecture.PEP10
            abstraction: Abstraction.ASMB5
            source: "image://icons/cards/p10_asmb5.svg"
            complete: true
            description: "Develop and debug assembly language programs alongside Pep/10's operating system. This level enables you to utilize OS features using system calls for enhanced functionality."
        }
        ListElement {
            text: "Pep/10, OS4"
            architecture: Architecture.PEP10
            abstraction: Abstraction.OS4
            complete: false
            partiallyComplete: true
        }
        ListElement {
            text: "Mc2, 1-byte bus"
            architecture: Architecture.PEP10
            abstraction: Abstraction.MC2
            source: "image://icons/cards/p10_mc2_1byte.svg"
            complete: false
        }
        ListElement {
            text: "Mc2, 2-byte bus"
            architecture: Architecture.PEP10
            abstraction: Abstraction.MC2
            complete: false
        }

        // Pep/9
        ListElement {
            text: "ISA3"
            architecture: Architecture.PEP9
            abstraction: Abstraction.ISA3
            complete: true
            source: "image://icons/cards/p9_isa3.svg"
            description: "Develop and debug machine language programs."
        }

        ListElement {
            text: "Asmb5"
            architecture: Architecture.PEP9
            abstraction: Abstraction.ASMB5
            complete: true
            source: "image://icons/cards/p9_asmb5.svg"
            description: "Develop and debug assembly language programs alongside Pep/9's operating system. This level enables you to utilize OS features using trap instructions for enhanced functionality."
        }
        ListElement {
            text: "OS4"
            architecture: Architecture.PEP9
            abstraction: Abstraction.OS4
            complete: false
        }
        ListElement {
            text: "Mc2, 1-byte bus"
            architecture: Architecture.PEP9
            abstraction: Abstraction.MC2
            source: "image://icons/cards/p10_mc2_1byte.svg"
            complete: false
        }
        ListElement {
            text: "Mc2, 2-byte bus"
            architecture: Architecture.PEP9
            abstraction: Abstraction.MC2
            complete: false
        }

        // Pep/8
        ListElement {
            text: "ISA3"
            architecture: Architecture.PEP8
            abstraction: Abstraction.ISA3
            complete: false
            source: "image://icons/cards/p9_isa3.svg"
        }
        ListElement {
            text: "Asmb5"
            architecture: Architecture.PEP8
            abstraction: Abstraction.ASMB5
            complete: false
            source: "image://icons/cards/p9_asmb5.svg"
        }
        ListElement {
            text: "OS4"
            architecture: Architecture.PEP8
            abstraction: Abstraction.OS4
            complete: false
        }

        ListElement {
            text: "Mc2, 1-byte bus"
            architecture: Architecture.PEP8
            abstraction: Abstraction.MC2
            source: "image://icons/cards/p10_mc2_1byte.svg"
            complete: false
        }

        // RISC-V
        ListElement {
            text: "Asmb3, bare metal"
            architecture: Architecture.RISCV
            abstraction: Abstraction.ASMB3
            complete: false
        }
    }
}
