import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    id: root
    color: palette.base
    required property var stackTracer
    signal updateGUI
    NuAppSettings {
        id: settings
    }

    TextMetrics {
        id: tm
        font: settings.extPalette.baseMono.font
        text: "W" // Dummy value to get width of widest character

        //  Calculate widths and height based on current font
        //  All column and line sizing are determined in this block.
        property double addressWidth: tm.width * 8
        property double valueWidth: tm.width * 8
        property double lineHeight: tm.height + 4 // Allow space around text
        property double boldBorderWidth: 4
    }
    ScopedActivationModel {
        id: stackActModel
        sourceModel: RootActivationModel {
            id: allStacksModel
            stackTracer: root.stackTracer
            Component.onCompleted: {
                root.updateGUI.connect(allStacksModel.update_volatile_values);
            }
        }
        scopeToIndex: allStacksModel.activeStackIndex
    }

    component NonStackRenderer: ColumnLayout {
        id: del
        required property string name
        Label {
            //Layout.leftMargin: tm.addressWidth + (tm.valueWidth - implicitWidth) / 2
            Layout.alignment: Qt.AlignHCenter & Qt.AlignVCenter
            //Layout.bottomMargin: -10
            text: del.name
            //visible: localStack.implicitHeight > 0
            Layout.preferredHeight: visible ? implicitHeight : 0
            font.family: tm.font.family
            font.pointSize: tm.font.pointSize * 1.5
            font.bold: true
        }
        Rectangle {
            Layout.alignment: Qt.AlignHCenter & Qt.AlignVCenter
            height: 80
            width: 100
            color: "orange"
            Text {
                anchors.centerIn: parent
                text: "Coming soon"
                color: "black"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                width: parent.width * 0.8
            }
            z: 10
        }
    }
    component StackRenderer: ColumnLayout {
        id: del
        required property string name
        spacing: 0
        MemoryStack {
            id: localStack
            // Because of negative spacing inside, top rect clips tab bar. Add margin to avoid clipping.
            Layout.topMargin: 4
            Layout.preferredHeight: implicitHeight
            Layout.preferredWidth: childrenRect.width
            Layout.minimumWidth: 250
            Layout.bottomMargin: 15

            //  Font and dimensions - Globals
            font: tm.font
            implicitAddressWidth: tm.addressWidth
            implicitValueWidth: tm.valueWidth
            implicitLineHeight: tm.lineHeight
            boldBorderWidth: tm.boldBorderWidth

            itemModel: stackActModel
        }
        StackGroundGraphic {
            id: graphic
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignHCenter & Qt.AlignBottom
            Layout.leftMargin: tm.addressWidth - tm.boldBorderWidth / 2
            // Honestly not sure why I need -1 here, but if I don't include the offset it looks wrong.
            Layout.preferredWidth: tm.valueWidth + tm.boldBorderWidth - 1
            //  Force graphic to be same width as value column
            graphicWidth: tm.valueWidth + tm.boldBorderWidth - 1
            Layout.preferredHeight: 0
        }
        Label {
            Layout.leftMargin: tm.addressWidth + (tm.valueWidth - implicitWidth) / 2
            Layout.alignment: Qt.AlignHCenter & Qt.AlignVCenter
            Layout.bottomMargin: -10
            text: del.name
            visible: localStack.implicitHeight > 0
            Layout.preferredHeight: visible ? implicitHeight : 0
            font.family: tm.font.family
            font.pointSize: tm.font.pointSize * 1.5
            font.bold: true
        }
        Item {
            Layout.preferredHeight: 15
        }
    }

    ScrollView {
        id: sv

        anchors.fill: parent
        topPadding: 8
        bottomPadding: 0
        contentWidth: column.width // The important part
        contentHeight: Math.max(column.implicitHeight, root.height - sv.topPadding - sv.bottomPadding) // Same
        clip: true // Prevent drawing column outside the scrollview borders
        spacing: 0

        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        // Construct the columns (globals/stack, heap) individually, then align those columns in a row.
        // This ordering allows the heap to "grow" into the horizontal space next to the stack.
        // The old Column then Row ordering would cause a scrollbar to appear if either the stack or heap was large.
        RowLayout {
            id: column
            anchors.fill: parent
            width: Math.max(implicitWidth, root.width)
            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                NonStackRenderer {
                    name: "Globals"
                }
                Item {
                    Layout.fillHeight: true

                    Layout.preferredHeight: 41
                    Layout.minimumHeight: 41
                }
                StackRenderer {
                    name: "Stack"
                }
            } // ColumnLayout

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                NonStackRenderer {
                    name: "Heap"
                }
            } // ColumnLayout
        } // RowLayout

    } //  ScrollView
}
