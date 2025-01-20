import QtQuick
import QtQuick.Controls
import QtQml.Models

Item {
    id: root
    property alias model: wrapper.model
    NuAppSettings {
        id: settings
    }
    TextMetrics {
        id: tm
        font: settings.extPalette.baseMono.font
        text: "W" // Dummy value to get width of widest character
    }
    Rectangle {

        color: palette.base
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
            left: parent.left
            right: parent.right
            top: parent.top
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
                leftPadding: 5

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
                rightPadding: 5

                color: palette.text
                text: "Value"
                font: tm.font
            }
        }
    }

    TableView {
        id: wrapper
        anchors {
            top: horizontalHeader.bottom
            bottom: parent.bottom
            right: parent.right
            rightMargin: vsc.width
            left: parent.left
        }
        contentWidth: width
        columnSpacing: tm.width * 4
        clip: true
        focus: true
        MouseArea {
            anchors.fill: parent
            onWheel: function (event) {
                if (event.angleDelta.y > 0) {
                    vsc.decrease()
                } else {
                    vsc.increase()
                }
            }
            onPressed: function (event) {
                const cell = wrapper.cellAtPosition(event.x, event.y)
                const index = wrapper.modelIndex(cell)
                const m = wrapper.model
                const sm = wrapper.selectionModel
                if (event.button === Qt.LeftButton) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        const pr = sm.currentIndex
                        if (pr.valid)
                            m.selectRectangle(sm, pr, index)
                    } else {
                        // Must use this variant. Setting current flag does not set currentIndex.
                        sm.setCurrentIndex(
                                    index,
                                    ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Current)
                    }
                }
            }
        }
        columnWidthProvider: function (index) {
            const header = "Symbol  Value".length
            const row = model.longest + 4 + 2 // Symbol + space + hex value
            return tm.width * Math.max(header, row) + 10
        }
        rowHeightProvider: function (index) {
            return tm.font.pixelSize + 4
        }
        onWidthChanged: {
            const actualSize = columnWidthProvider(0) + columnSpacing
            wrapper.model.setColumnCount(width / actualSize)
        }
        onModelChanged: {
            const actualSize = columnWidthProvider(0) + columnSpacing
            wrapper.model.setColumnCount(width / actualSize)
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
            model: wrapper.model
        }
        delegate: Rectangle {
            id: delegate
            required property bool selected
            required property bool current
            implicitHeight: symbol.contentHeight
            color: selected ? palette.highlight : "transparent"

            Label {
                id: symbol
                focus: false
                anchors.left: parent.left
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                leftPadding: 5

                color: palette.text
                text: model.symbol
                font: tm.font
            }
            Label {
                id: value
                focus: false
                anchors.left: symbol.right
                anchors.right: parent.right
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                rightPadding: 5

                color: palette.text
                text: model.value
                font: tm.font
            }
        }
    }
}
