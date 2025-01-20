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

    TableView {
        id: wrapper
        anchors.fill: parent
        anchors.leftMargin: vsc.width
        contentWidth: width
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
            return tm.width * 15 + 10
        }
        rowHeightProvider: function (index) {
            return tm.font.pixelSize + 4
        }
        onWidthChanged: {
            wrapper.model.setColumnCount(width / columnWidthProvider(0))
        }
        onModelChanged: {
            wrapper.model.setColumnCount(width / columnWidthProvider(0))
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
            implicitHeight: symbol.contentHeight + value.contentHeight
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
