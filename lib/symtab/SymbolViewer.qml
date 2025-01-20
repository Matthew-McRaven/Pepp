import QtQuick 2.15
import QtQuick.Controls

Rectangle {
    NuAppSettings {
        id: settings
    }

    color: palette.base
    property alias model: wrapper.model
    //  Give object code viewer a background box
    border.width: 1
    border.color: palette.mid
    GridView {
        id: wrapper
        anchors.fill: parent
        anchors.leftMargin: vsc.width
        cellHeight: tm.height
        cellWidth: tm.width * 15 + 10 // (4 + (tm.longest == 0 ? 10 : tm.longest)) + 10
        clip: true
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

        Component.onCompleted: {

            //tm.longest = model.longest
            console.log("Cell width: " + cellWidth)
        }

        TextMetrics {
            id: tm
            font: settings.extPalette.baseMono.font
            text: "W" // Dummy value to get width of widest character
        }

        delegate: SymbolCell {
            width: wrapper.cellWidth
            height: wrapper.cellHeight

            symbolText: model.symbol
            valueText: model.value
            valueWidth: tm.width * 4

            textColor: palette.text
            font: tm.font
        }
    }
}
