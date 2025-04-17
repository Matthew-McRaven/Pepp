import QtQuick
import QtQuick.Controls
import QtQml.Models

Item {
    id: root
    property alias model: filterModel.sourceModel
    property alias scopeFilter: filterModel.scopeFilter
    NuAppSettings {
        id: settings
    }
    TextMetrics {
        id: tm
        font: settings.extPalette.baseMono.font
        text: "W" // Dummy value to get width of widest character
    }
    Rectangle {
        id: outline

        anchors.fill: parent
        //  Give object code viewer a background box
        border.width: 1
        border.color: palette.mid
    }
    HorizontalHeaderView {
        id: horizontalHeader
        // Dummy value to silence warning about non-existent role.
        textRole: "symbol"
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: outline.border.width
        }
        syncView: wrapper
        clip: true
        delegate: Item {
            id: headerDelegate
            implicitHeight: symbolHead.contentHeight
            Label {
                id: symbolHead
                anchors.left: parent.left
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                leftPadding: tm.width * 2

                color: palette.text
                text: "Symbol"
                font: tm.font
            }
            Label {
                id: valueHead
                focus: false
                anchors.left: symbolHead.right
                anchors.right: parent.right
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                rightPadding: tm.width * 2

                color: palette.text
                text: "Value"
                font: tm.font
            }
        }
        Rectangle {
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                rightMargin: vsc.width
            }
            color: palette.midlight
            z: -1
        }
    }
    TableView {
        id: wrapper
        anchors {
            top: horizontalHeader.bottom
            topMargin: rowHeightProvider(0) * 0.15
            bottom: parent.bottom
            right: parent.right
            rightMargin: vsc.width
            left: parent.left
        }
        model: StaticSymbolReshapeModel {
            id: reshapeModel
            sourceModel: StaticSymbolFilterModel {
                id: filterModel
            }
        }

        contentWidth: width
        clip: true
        focus: true
        MouseArea {
            anchors.fill: parent
            onPressed: function (event) {
                const cell = wrapper.cellAtPosition(event.x, event.y)
                const index = wrapper.modelIndex(cell)
                const m = wrapper.model
                const sm = wrapper.selectionModel
                if (event.button === Qt.LeftButton) {
                    const flags = ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Current
                    if (event.modifiers & Qt.ShiftModifier) {
                        const pr = sm.currentIndex
                        if (pr.valid)
                            m.selectRectangle(sm, pr, index)
                    } else {
                        // Must use this variant. Setting current flag does not set currentIndex.
                        sm.setCurrentIndex(index, flags)
                    }
                    // Must force tableview to have focus, else copy will not work
                    wrapper.forceActiveFocus()
                }
            }
        }

        columnWidthProvider: function (index) {
            const header = "Symbol  Value".length + 4 // Need 2 padding  on each side
            const row = model.longest + 4 + 2 // Symbol + space + hex value
            return tm.width * Math.max(header, row) + 10
        }
        rowHeightProvider: function (index) {
            return tm.font.pixelSize + 4
        }
        onWidthChanged: {
            const actualSize = columnWidthProvider(0) + columnSpacing
            reshapeModel.setColumnCount(width / actualSize)
        }
        onModelChanged: {
            const actualSize = columnWidthProvider(0) + columnSpacing
            reshapeModel.setColumnCount(width / actualSize)
        }
        function copy() {
            reshapeModel.copy(selectionModel.selectedIndexes)
        }

        boundsBehavior: Flickable.StopAtBounds

        //  Disable horizontal scroll bar
        ScrollBar.horizontal: ScrollBar {
            policy: ScrollBar.AlwaysOff
        }

        //  Enable vertical scroll bar-always on
        ScrollBar.vertical: ScrollBar {
            id: vsc
            policy: ScrollBar.AlwaysOn
        }

        selectionModel: ItemSelectionModel {
            model: reshapeModel
        }
        delegate: Rectangle {
            id: delegate
            required property bool selected
            required property bool current
            required property string symbol
            required property string value

            implicitHeight: symbol.contentHeight
            color: selected ? palette.highlight : "transparent"
            focus: false
            Label {
                id: symbol
                focus: false
                anchors.left: parent.left
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                leftPadding: tm.width * 2

                color: palette.text
                text: delegate.symbol
                font: tm.font
            }
            Label {
                id: value
                focus: false
                anchors.left: symbol.right
                anchors.right: parent.right
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                rightPadding: tm.width * 2

                color: palette.text
                text: delegate.value
                font: tm.font
            }
        }
    }
}
