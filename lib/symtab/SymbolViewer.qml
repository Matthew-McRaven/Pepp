import QtQuick 2.15
import QtQuick.Controls

TableView {
    id: table
    delegate: Text {
        text: model.display
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
    }
}
