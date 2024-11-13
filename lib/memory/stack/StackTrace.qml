import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    id: root
    color: palette.base

    TextMetrics {
        id: tm
        font: Theme.font
        text: "W" // Dummy value to get width of widest character

        //  Calculate widths and height based on current font
        //  All column and line sizing are determined in this
        //  block.
        property double addressWidth: tm.width * 8
        property double valueWidth: tm.width * 8
        property double lineHeight: tm.height + 4 // Allow space around text
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
                Layout.preferredHeight: implicitHeight
                Layout.preferredWidth: globals.childrenRect.width

                //  Font and dimensions - Globals
                font: tm.font
                implicitAddressWidth: tm.addressWidth
                implicitValueWidth: tm.valueWidth
                implicitLineHeight: tm.lineHeight

                visible: activationModel
                itemModel: activationModel
            }
            MemorySpacer {
                id: globalSpacer
                Layout.fillHeight: false
                Layout.leftMargin: tm.addressWidth
                Layout.preferredWidth: tm.valueWidth
                Layout.alignment: Qt.AlignHCenter & Qt.AlignVCenter
                Layout.preferredHeight: globalSpacer.ellipsisHeight

                ellipsisSize: 7.5
            }
            MemoryStack {
                id: heap
                Layout.fillHeight: false
                Layout.preferredHeight: implicitHeight

                //  Font and dimensions - Heap
                font: tm.font
                implicitAddressWidth: tm.addressWidth
                implicitValueWidth: tm.valueWidth
                implicitLineHeight: tm.lineHeight

                itemModel: activationModel
            }
            MemorySpacer {
                id: heapSpacer
                Layout.fillHeight: true
                Layout.leftMargin: tm.addressWidth
                Layout.preferredWidth: tm.valueWidth
                Layout.alignment: Qt.AlignHCenter & Qt.AlignVCenter

                Layout.preferredHeight: 41
                Layout.minimumHeight: 41

                ellipsisSize: 7.5
            }

            MemoryStack {
                id: stack
                Layout.fillHeight: false
                Layout.preferredHeight: implicitHeight

                //  Font and dimensions - Stack
                font: tm.font
                implicitAddressWidth: tm.addressWidth
                implicitValueWidth: tm.valueWidth
                implicitLineHeight: tm.lineHeight

                itemModel: activationModel
            }
            GraphicSpacer {
                id: graphic
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignHCenter & Qt.AlignVCenter
                Layout.leftMargin: tm.addressWidth
                Layout.preferredWidth: tm.valueWidth

                height: 20

                //  Force graphic to be same width as value column
                graphicWidth: tm.valueWidth
            }
        } //  ColumnLayout
    } //  ScrollView
}
