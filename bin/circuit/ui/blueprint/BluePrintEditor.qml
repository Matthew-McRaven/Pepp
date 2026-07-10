//pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root

    ListView {
        id: clockview
        anchors.fill: parent
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.ApplyRange

        delegate: Row {
            Text {
                text: name
            }
        }
        model: ListModel {
            ListElement {
                name: "And"
            }
            ListElement {
                name: "Or"
            }
            ListElement {
                name: "Inverter"
            }
            ListElement {
                name: "Nand"
            }
            ListElement {
                name: "Nor"
            }
        }
    }
}
