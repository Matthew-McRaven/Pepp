import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    id: root
    color: palette.base

    //border.color: "red"
    //border.width: 1
    TextMetrics {
        id: tm
        font: Theme.font
        text: "W" // Dummy value to get width of widest character
    }
    // Create C++ items using the magic of QQmlPropertyList and DefaultProperty
    ActivationModel {
        id: activationModel
        ActivationRecord {
            active: false
            RecordLine {
                address: 0
                value: "10"
                status: ChangeType.Allocated
                name: "a"
            }
            RecordLine {
                address: 2
                value: "20"
                status: ChangeType.Modified
                name: "b"
            }
        }
        ActivationRecord {
            active: true
            RecordLine {
                address: 0x4a
                value: "30"
                name: "c1"
            }
            RecordLine {
                address: 0x4b
                value: "32"
                name: "c2"
            }
        }
        ActivationRecord {
            active: true
            RecordLine {
                address: 6
                value: "40"
                status: ChangeType.Modified
                name: "d"
            }
            RecordLine {
                address: 7
                value: "50"
                name: "e"
            }
            RecordLine {
                address: 9
                value: "60"
                status: ChangeType.Modified
                name: "f"
            }
        }
    }

    ScrollView {
        id: sv
        anchors.fill: parent
        topPadding: 8
        bottomPadding: 0
        contentWidth: column.width // The important part
        contentHeight: Math.max(
                           column.implicitHeight,
                           root.height - sv.topPadding - sv.bottomPadding) // Same
        clip: true // Prevent drawing column outside the scrollview borders
        spacing: 0

        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: column
            anchors.fill: parent
            width: Math.max(implicitWidth, root.width)
            MemoryStack {
                id: globals
                Layout.fillHeight: false
                // Because of negative spacing inside, top rect clips tab bar. Add margin to avoid clipping.
                Layout.topMargin: 4
                font: tm.font
                visible: activationModel
                itemModel: activationModel
            }
            MemorySpacer {
                id: globalSpacer
                Layout.fillHeight: false
                Layout.preferredWidth: root.width - column.margins * 2
                Layout.alignment: Qt.AlignLeft & Qt.AlignVCenter
                Layout.leftMargin: tm.width * 8 + 15
                Layout.preferredHeight: globalSpacer.ellipsisHeight

                ellipsisSize: 7.5
            }
            MemoryStack {
                id: heap
                Layout.fillHeight: false
                font: tm.font
                itemModel: activationModel
            }
            MemorySpacer {
                id: heapSpacer
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignLeft & Qt.AlignVCenter
                Layout.leftMargin: tm.width * 8 + 15
                Layout.preferredWidth: root.width - column.margins * 2
                Layout.preferredHeight: 41
                Layout.minimumHeight: 41

                ellipsisSize: 7.5
            }

            MemoryStack {
                id: stack
                Layout.fillHeight: false
                font: tm.font
                itemModel: activationModel
            }
            GraphicSpacer {
                id: graphic
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignLeft & Qt.AlignVCenter
                Layout.leftMargin: tm.width * 8 + 48
                Layout.preferredWidth: root.width - column.margins * 2
                height: 20
            }
        } //  ColumnLayout
    } //  ScrollView


    /*    Component.onCompleted: {
        console.log("Main window Height / Width: " + root.height + "/" + root.width)
        console.log("ColumnLayout Height / Width: " + column.height + "/" + column.width)
        console.log("Globals Height / Width: " + globals.height + "/" + globals.width)
        console.log("Global Space Height / Width: " + globalSpacer.height + "/"
                    + globalSpacer.width)
        console.log("Heap Height / Width: " + heap.height + "/" + heap.width)
        console.log("Heap Space Height / Width: " + heapSpacer.height + "/" + heapSpacer.width)
        console.log("Stack Height / Width: " + stack.height + "/" + stack.width)
        console.log("Graphic Height / Width: " + graphic.height + "/" + graphic.width)
    } */
}
