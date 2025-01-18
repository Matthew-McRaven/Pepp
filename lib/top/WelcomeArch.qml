import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    NuAppSettings {
        id: settings
    }
    FontMetrics {
        id: fm
        font.pointSize: 48
    }

    GridLayout {
        id: images
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            leftMargin: 20
            rightMargin: 20
        }
        property int selectedIndex: -1 // Tracks the currently selected image index
        columns: parent.width < 700 ? 1 : 2
        columnSpacing: 20
        Label {
            Layout.columnSpan: images.columns
            text: "Choose an Architecture"
            font: fm.font
        }

        Repeater {
            model: elements
            Item {
                // Estimating 20 pixels of padding per column for spacing / margins / etc
                property real maxColumnWidth: (root.width - 60) / images.columns
                property real maxWidth: Math.min(800, maxColumnWidth)
                // Source image is 1000x175. Must explicitly set max height or their will be excessive white space.
                property real maxHeight: Math.min(175. / 1000. * maxWidth)
                Layout.maximumHeight: maxHeight
                Layout.maximumWidth: maxWidth
                implicitHeight: maxHeight
                implicitWidth: maxWidth
                Image {
                    anchors {
                        fill: parent
                    }
                    source: model.icon
                    fillMode: Image.PreserveAspectFit
                }
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: index === images.selectedIndex ? palette.highlight : "transparent"
                    border.width: 3
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        settings.general.defaultArch = model.archCode
                    }
                }
                Component.onCompleted: {
                    if (model.archCode === settings.general.defaultArch) {
                        images.selectedIndex = Qt.binding(() => index)
                    }
                }
                Connections {
                    target: settings.general
                    function onDefaultArchChanged() {
                        if (model.archCode == settings.general.defaultArch) {
                            images.selectedIndex = Qt.binding(() => index)
                            root.goForward()
                        }
                    }
                }
            }
        }
    }

    // Going forward, I would like there to be a bulleted list describing some features of the architecture below image area.
    // I also want a "confirm" button so that you can read the details before accepting
    // Lastly, I want to be able to filter the architectures based on which ones are not WIP, like the project selection.
    signal goForward
    ListModel {
        id: elements
        ListElement {
            name: "Pep/10"
            description: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla nec odio nec turpis luctus tincidunt. Sed auctor, justo nec ultricies tincidunt, elit purus tincidunt purus, nec ultricies odio odio vel nunc."
            complete: true
            archCode: Architecture.PEP10
            icon: "image://icons/arch/p10.svg"
        }
        ListElement {
            name: "RISC-V"
            description: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla nec odio nec turpis luctus tincidunt. Sed auctor, justo nec ultricies tincidunt, elit purus tincidunt purus, nec ultricies odio odio vel nunc."
            complete: false
            archCode: Architecture.RISCV
            icon: "image://icons/arch/riscv-color.svg"
        }
        ListElement {
            name: "Pep/9"
            description: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla nec odio nec turpis luctus tincidunt. Sed auctor, justo nec ultricies tincidunt, elit purus tincidunt purus, nec ultricies odio odio vel nunc."
            complete: true
            archCode: Architecture.PEP9
            icon: "image://icons/arch/p9.svg"
        }
        ListElement {
            name: "Pep/8"
            description: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla nec odio nec turpis luctus tincidunt. Sed auctor, justo nec ultricies tincidunt, elit purus tincidunt purus, nec ultricies odio odio vel nunc."
            complete: false
            archCode: Architecture.PEP8
            icon: "image://icons/arch/p8.svg"
        }
    }
}
