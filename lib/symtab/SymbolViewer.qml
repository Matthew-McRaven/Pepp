import QtQuick 2.15
import QtQuick.Controls

GridView {
    id: wrapper
    anchors.fill: parent
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
        font: Theme.font
        text: "W" // Dummy value to get width of widest character

        //property int longest: 0
    }

    delegate: SymbolCell {
        width: wrapper.cellWidth
        height: wrapper.cellHeight

        symbolText: model.symbol
        valueText: model.value
        //symbolWidth: wrapper.cellWidth - valueWidth //(tm.longest == 0 ? 10 : tm.longest)
        valueWidth: tm.width * 4

        backgroundColor: (model.index % 2) ? palette.button : palette.base
        textColor: palette.text
        font: tm.font
    }
}
