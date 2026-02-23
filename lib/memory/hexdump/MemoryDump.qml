import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
//  For DelegateChooser
import QtQml.Models

import "." as Ui
import edu.pepp 1.0

Item {
    id: root
    NuAppSettings {
        id: settings
    }
    FontMetrics {
        id: fm
        font: settings.extPalette.baseMono.font
    }
    property int colWidth: 25
    property int rowHeight: 20
    property alias model: tableView.model
    property alias memory: memory.memory
    property alias mnemonics: memory.mnemonics
    property int currentAddress: tableView.currentAddress
    property alias bytesPerRow: memory.bytesPerRow
    function scrollToAddress(addr) {
        const row = Math.floor(addr / root.model?.bytesPerRow);
        tableView.positionViewAtRow(row, TableView.Contain);
    }
    implicitWidth: Math.max(tableView.implicitWidth, buttonLayout.implicitWidth)

    TableView {
        id: tableView
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: buttonLayout.top

        rowSpacing: 0
        columnSpacing: 0
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        focus: true

        //  Selection information
        selectionBehavior: TableView.SelectCells
        selectionMode: TableView.ContiguousSelection
        model: MemoryModel {
            id: memory
        }

        //  Ascii column must be calculated since byte width per line is configurable
        property int asciiWidth: 10 * (root.model?.bytesPerRow ?? 10)

        //  Used for paging
        property int currentAddress: (topRow > 0 ? topRow : 0) * (root.model?.bytesPerRow ?? 0)

        //  For grid column sizes. Currently, columns are not resizeable
        columnWidthProvider: function (column) {
            switch (column) {
            case root.model.Column.LineNo:
                return 40; //colWidth * 2
            case root.model.Column.Border1:
                return 11;
            case root.model.Column.Border2:
                return 11;
            case root.model.Column.Ascii:
                return asciiWidth;
            default:
                return root.colWidth;
            }
        }
        rowHeightProvider: function (row) {
            return rowHeight;
        }

        //  Disable horizontal scroll bar
        ScrollBar.horizontal: ScrollBar {
            policy: ScrollBar.AlwaysOff
        }

        //  Enable vertical scroll bar-always on
        ScrollBar.vertical: ScrollBar {
            id: vsc
            policy: ScrollBar.AlwaysOn
        }

        //  Used for drawing grid
        delegate: memoryDelegateChooser
        // Allow access to changing colors in RO delegate w/o local state.
        // Sensitive to the order or roles in rawmemory.
        property list<color> mapped_colors: Array(settings.extPalette.base.background // None
        , settings.extPalette.error.background // Modified
        , settings.extPalette.linkVisited.foreground // SP
        , settings.extPalette.link.foreground // PC
        )
        DelegateChooser {
            id: memoryDelegateChooser
            role: "type"

            DelegateChoice {
                id: border
                roleValue: "border"
                Ui.MemoryDumpBorder {
                    backgroundColor: palette.base
                    foregroundColor: palette.text
                    implicitWidth: colWidth
                    implicitHeight: rowHeight
                }
            }

            //  Default -- line numbers and ASCII
            DelegateChoice {
                Ui.MemoryDumpReadOnly {
                    required property int highlight
                    required property string display
                    required property string tooltip
                    required property int textAlign
                    backgroundColor: tableView.mapped_colors[highlight ?? 0]
                    textColor: palette.text
                    text: display
                    tooltip: tooltip ?? null
                    textAlign: textAlign
                    font: fm.font
                    implicitWidth: colWidth
                    implicitHeight: rowHeight
                }
            }
        }
    }
    GridLayout {
        id: buttonLayout
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        columns: root.width > (4 * pcButton.width) ? 4 : 2
        Label {
            text: "Scroll to:"
            color: palette.windowText
        }
        TextField {
            id: addrField
            maximumLength: 6
            Layout.minimumWidth: fm.averageCharacterWidth * (maximumLength + 2)
            Layout.maximumWidth: pcButton.width
            font: fm.font
            text: `0x${root.currentAddress.toString(16).padStart(4, '0').toUpperCase()}`
            validator: RegularExpressionValidator {
                regularExpression: /0x[0-9a-fA-F]{1,4}/
            }
            onEditingFinished: {
                const v = parseInt(text, 16);
                if (v < 0 || v > 0xFFFF)
                    text = "0x0000";
                else
                    root.scrollToAddress(v);
            }
        }
        Button {
            id: spButton
            text: "SP"
            onClicked: root.scrollToAddress(root.memory.sp())
        }
        Button {
            id: pcButton
            text: "PC"
            onClicked: root.scrollToAddress(root.memory.pc())
        }
    }
}
